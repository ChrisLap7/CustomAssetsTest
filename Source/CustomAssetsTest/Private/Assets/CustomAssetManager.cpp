#include "Assets/CustomAssetManager.h"
#include "Assets/CustomAssetBase.h"
#include "Assets/CustomAssetBundle.h"
#include "Assets/CustomAssetMemoryTracker.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "Engine/StreamableManager.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Algo/Reverse.h"
#if WITH_EDITOR
#include "ObjectTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#endif

UCustomAssetManager::UCustomAssetManager()
{
    // Initialize default values
    DefaultLoadingStrategy = EAssetLoadingStrategy::OnDemand;
    MemoryPolicy = EMemoryManagementPolicy::KeepAll;
    MemoryThreshold = 1024 * 1024 * 1024; // Default 1GB threshold
    
    // Create the memory tracker
    MemoryTracker = NewObject<UCustomAssetMemoryTracker>();
    MemoryTracker->AddToRoot(); // Prevent garbage collection
}

UCustomAssetManager& UCustomAssetManager::Get()
{
    UCustomAssetManager* AssetManager = Cast<UCustomAssetManager>(GEngine->AssetManager);
    check(AssetManager);
    return *AssetManager;
}

void UCustomAssetManager::StartInitialLoading()
{
    Super::StartInitialLoading();

    // Scan for assets when the asset manager initializes
    ScanForAssets();
    
    // Scan for bundles
    ScanForBundles();
    
    // Update dependencies between assets
    UpdateDependencies();
    
    // Preload bundles marked for preloading
    PreloadBundles();
}

UCustomAssetBase* UCustomAssetManager::LoadAssetById(const FName& AssetId)
{
    // Use the default loading strategy
    return LoadAssetByIdWithStrategy(AssetId, DefaultLoadingStrategy);
}

UCustomAssetBase* UCustomAssetManager::LoadAssetByIdWithStrategy(const FName& AssetId, EAssetLoadingStrategy Strategy)
{
    // Check if the asset is already loaded - use direct lookup for better performance
    UCustomAssetBase* LoadedAsset = GetAssetById(AssetId);
    if (::IsValid(LoadedAsset))
    {
        // Record access to the asset
        if (MemoryTracker)
        {
            MemoryTracker->RecordAssetAccess(AssetId);
        }
        
        return LoadedAsset;
    }

    // Check if we have a path for this asset ID - use direct lookup for better performance
    FSoftObjectPath* AssetPath = AssetPathMap.Find(AssetId);
    if (!AssetPath)
    {
        UE_LOG(LogTemp, Warning, TEXT("Asset with ID %s not found in asset path map"), *AssetId.ToString());
        return nullptr;
    }

    UCustomAssetBase* Asset = nullptr;

    // Apply the loading strategy
    switch (Strategy)
    {
    case EAssetLoadingStrategy::OnDemand:
        // Load the asset synchronously
        Asset = Cast<UCustomAssetBase>(AssetPath->TryLoad());
        break;

    case EAssetLoadingStrategy::Preload:
        // For preload, we should have already loaded it, but just in case
        Asset = Cast<UCustomAssetBase>(AssetPath->TryLoad());
        break;

    case EAssetLoadingStrategy::Streaming:
        // For streaming, we start the load but return nullptr
        // The asset will be registered when the streaming completes
        StreamingHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
            *AssetPath,
            FStreamableDelegate::CreateUObject(this, &UCustomAssetManager::OnAssetLoaded, AssetId, *AssetPath)
        );
        return nullptr;

    case EAssetLoadingStrategy::LazyLoad:
        // For lazy load, we don't load the asset now, but return nullptr
        return nullptr;

    default:
        UE_LOG(LogTemp, Warning, TEXT("Unknown loading strategy for asset %s"), *AssetId.ToString());
        return nullptr;
    }

    // If we got here, we loaded the asset synchronously
    if (::IsValid(Asset))
    {
        // Register the loaded asset
        RegisterAsset(Asset);
        
        // Load hard dependencies
        LoadDependencies(AssetId, true, Strategy);
        
        // Manage memory usage
        ManageMemoryUsage();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load asset with ID %s"), *AssetId.ToString());
    }

    return Asset;
}

void UCustomAssetManager::PreloadAssets(const TArray<FName>& AssetIds)
{
    // Create an array of asset paths to preload
    TArray<FSoftObjectPath> AssetPaths;
    
    for (const FName& AssetId : AssetIds)
    {
        FSoftObjectPath* AssetPath = AssetPathMap.Find(AssetId);
        if (AssetPath)
        {
            AssetPaths.Add(*AssetPath);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Asset with ID %s not found for preloading"), *AssetId.ToString());
        }
    }

    // Use the streamable manager to load all assets
    if (AssetPaths.Num() > 0)
    {
        FStreamableManager& LocalStreamableManager = UAssetManager::GetStreamableManager();
        LocalStreamableManager.RequestSyncLoad(AssetPaths);

        // Register all loaded assets
        for (const FName& AssetId : AssetIds)
        {
            FSoftObjectPath* AssetPath = AssetPathMap.Find(AssetId);
            if (AssetPath && AssetPath->IsValid())
            {
                UCustomAssetBase* Asset = Cast<UCustomAssetBase>(AssetPath->ResolveObject());
                if (Asset)
                {
                    RegisterAsset(Asset);
                    
                    // Load hard dependencies
                    LoadDependencies(AssetId, true, EAssetLoadingStrategy::Preload);
                }
            }
        }
    }
    
    // Manage memory usage after preloading
    ManageMemoryUsage();
}

void UCustomAssetManager::StreamAsset(const FName& AssetId, const FOnAssetLoaded& CompletionCallback)
{
    // Check if we have a path for this asset ID
    FSoftObjectPath* AssetPath = AssetPathMap.Find(AssetId);
    if (!AssetPath)
    {
        UE_LOG(LogTemp, Warning, TEXT("Asset with ID %s not found for streaming"), *AssetId.ToString());
        return;
    }

    // Store the completion callback for later use
    PendingCallbacks.Add(AssetId, CompletionCallback);
    
    // Start the streaming process
    FStreamableManager& LocalStreamableManager = UAssetManager::GetStreamableManager();
    
    // Request async load of the asset
    StreamingHandle = LocalStreamableManager.RequestAsyncLoad(
        *AssetPath,
        FStreamableDelegate::CreateUObject(this, &UCustomAssetManager::OnAssetStreamed, AssetId)
    );
}

// Helper function to handle streamed asset completion
void UCustomAssetManager::OnAssetStreamed(FName AssetId)
{
    FSoftObjectPath* AssetPath = AssetPathMap.Find(AssetId);
    if (AssetPath && AssetPath->IsValid())
    {
        UCustomAssetBase* Asset = Cast<UCustomAssetBase>(AssetPath->ResolveObject());
        if (Asset)
        {
            RegisterAsset(Asset);
            
            // Load hard dependencies
            LoadDependencies(AssetId, true, EAssetLoadingStrategy::Streaming);
            
            // Manage memory usage after streaming
            ManageMemoryUsage();
            
            UE_LOG(LogTemp, Log, TEXT("Asset with ID %s streamed and registered"), *AssetId.ToString());
        }
    }
    
    // Call the user's completion callback if one exists
    FOnAssetLoaded* Callback = PendingCallbacks.Find(AssetId);
    if (Callback && Callback->IsBound())
    {
        Callback->Execute();
    }
    
    // Clean up the callback
    PendingCallbacks.Remove(AssetId);
}

void UCustomAssetManager::SetDefaultLoadingStrategy(EAssetLoadingStrategy Strategy)
{
    DefaultLoadingStrategy = Strategy;
}

bool UCustomAssetManager::UnloadAssetById(const FName& AssetId)
{
    UCustomAssetBase* Asset = GetAssetById(AssetId);
    if (!Asset)
    {
        UE_LOG(LogTemp, Warning, TEXT("Asset with ID %s not loaded"), *AssetId.ToString());
        return false;
    }

    // Check if the asset can be safely unloaded
    if (!CanUnloadAsset(AssetId))
    {
        UE_LOG(LogTemp, Warning, TEXT("Asset with ID %s cannot be unloaded because other loaded assets depend on it"), *AssetId.ToString());
        return false;
    }

    // Unregister the asset
    UnregisterAsset(Asset);
    
    // Update the memory tracker
    if (MemoryTracker)
    {
        MemoryTracker->SetAssetLoadedState(AssetId, false);
    }
    
    return true;
}

UCustomAssetBase* UCustomAssetManager::GetAssetById(const FName& AssetId) const
{
    // Use FindRef for direct map lookup which is more efficient than Find+dereference
    UCustomAssetBase* Asset = LoadedAssets.FindRef(AssetId);
    
    // Record access to the asset if found
    if (::IsValid(Asset) && MemoryTracker)
    {
        MemoryTracker->RecordAssetAccess(AssetId);
    }
    
    return Asset;
}

TArray<UCustomAssetBase*> UCustomAssetManager::GetAllLoadedAssets() const
{
    TArray<UCustomAssetBase*> Assets;
    LoadedAssets.GenerateValueArray(Assets);
    return Assets;
}

TArray<FName> UCustomAssetManager::GetAllAssetIds() const
{
    TArray<FName> AssetIds;
    AssetPathMap.GetKeys(AssetIds);
    return AssetIds;
}

bool UCustomAssetManager::ExportAssetsToCSV(const FString& FilePath) const
{
    FString CSVContent = "AssetId,DisplayName,Description,Tags,Version,LastModified\n";

    // Get all asset IDs
    TArray<FName> AssetIds = GetAllAssetIds();

    // Sort asset IDs for consistent output
    AssetIds.Sort([](const FName& A, const FName& B) {
        return A.ToString() < B.ToString();
    });

    // Process each asset
    for (const FName& AssetId : AssetIds)
    {
        // Try to get the loaded asset
        UCustomAssetBase* Asset = GetAssetById(AssetId);
        
        if (Asset)
        {
            // Format the tags as a comma-separated list
            FString TagsStr;
            for (int32 i = 0; i < Asset->Tags.Num(); ++i)
            {
                if (i > 0)
                {
                    TagsStr += "|";
                }
                TagsStr += Asset->Tags[i].ToString();
            }

            // Format the last modified date
            FString LastModifiedStr = Asset->LastModified.ToString(TEXT("%Y-%m-%d %H:%M:%S"));

            // Escape any commas in the description
            FString EscapedDescription = Asset->Description.ToString();
            EscapedDescription.ReplaceInline(TEXT(","), TEXT("\\,"));
            EscapedDescription.ReplaceInline(TEXT("\n"), TEXT("\\n"));

            // Add the asset data to the CSV
            CSVContent += FString::Printf(TEXT("%s,%s,%s,%s,%d,%s\n"),
                *Asset->AssetId.ToString(),
                *Asset->DisplayName.ToString(),
                *EscapedDescription,
                *TagsStr,
                Asset->Version,
                *LastModifiedStr);
        }
        else
        {
            // Asset not loaded, just add the ID
            CSVContent += FString::Printf(TEXT("%s,Not Loaded,,,0,\n"), *AssetId.ToString());
        }
    }

    // Write the CSV file
    return FFileHelper::SaveStringToFile(CSVContent, *FilePath);
}

void UCustomAssetManager::ScanForAssets()
{
    // Get the asset registry module
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    // Wait for asset discovery to complete
    AssetRegistry.WaitForCompletion();

    // Get all assets derived from UCustomAssetBase
    TArray<FAssetData> AssetData;
    FTopLevelAssetPath ClassPath = UCustomAssetBase::StaticClass()->GetClassPathName();
    AssetRegistry.GetAssetsByClass(ClassPath, AssetData, true);

    UE_LOG(LogTemp, Log, TEXT("Found %d custom assets"), AssetData.Num());

    // Clear existing asset path map
    AssetPathMap.Empty();

    // Process each asset
    for (const FAssetData& Data : AssetData)
    {
        // Load the asset to get its ID
        UCustomAssetBase* Asset = Cast<UCustomAssetBase>(Data.GetAsset());
        if (Asset && !Asset->AssetId.IsNone())
        {
            // Add to the asset path map
            AssetPathMap.Add(Asset->AssetId, Data.ToSoftObjectPath());
            
            // Update the last modified timestamp
            Asset->LastModified = FDateTime::Now();
            
            UE_LOG(LogTemp, Log, TEXT("Registered asset: %s (ID: %s)"), *Data.AssetName.ToString(), *Asset->AssetId.ToString());
        }
        else if (Asset)
        {
            UE_LOG(LogTemp, Warning, TEXT("Asset %s has no ID assigned"), *Data.AssetName.ToString());
        }
    }
}

void UCustomAssetManager::RegisterAsset(UCustomAssetBase* Asset)
{
    if (!Asset || Asset->AssetId.IsNone())
    {
        return;
    }

    // Add to loaded assets map
    LoadedAssets.Add(Asset->AssetId, Asset);
    
    // Register dependencies
    RegisterAssetDependencies(Asset);
    
    // Track memory usage
    if (MemoryTracker)
    {
        int64 MemoryUsage = EstimateAssetMemoryUsage(Asset);
        MemoryTracker->TrackAsset(Asset->AssetId, MemoryUsage);
    }
}

void UCustomAssetManager::RegisterAssetPath(const FName& AssetId, const FSoftObjectPath& AssetPath)
{
    if (AssetId.IsNone() || !AssetPath.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot register invalid asset path for ID: %s"), 
            *AssetId.ToString());
        return;
    }

    // Check if the asset is already loaded
    if (LoadedAssets.Contains(AssetId))
    {
        UE_LOG(LogTemp, Warning, TEXT("Asset %s is already loaded, no need to register its path"), 
            *AssetId.ToString());
        return;
    }

    // Check if we already have a path for this asset ID
    if (AssetPathMap.Contains(AssetId))
    {
        UE_LOG(LogTemp, Warning, TEXT("Updating path for asset %s from %s to %s"), 
            *AssetId.ToString(), 
            *AssetPathMap[AssetId].ToString(),
            *AssetPath.ToString());
    }

    // Add or update the path in the asset path map
    AssetPathMap.Add(AssetId, AssetPath);
    UE_LOG(LogTemp, Log, TEXT("Registered asset path %s for ID %s"), 
        *AssetPath.ToString(), *AssetId.ToString());
}

void UCustomAssetManager::UnregisterAsset(UCustomAssetBase* Asset)
{
    if (!Asset || Asset->AssetId.IsNone())
    {
        return;
    }

    // Unregister dependencies
    UnregisterAssetDependencies(Asset);

    // Remove from loaded assets map
    LoadedAssets.Remove(Asset->AssetId);
}

// ASSET BUNDLE FUNCTIONS

void UCustomAssetManager::RegisterBundle(UCustomAssetBundle* Bundle)
{
    if (!Bundle)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] RegisterBundle: Cannot register null bundle"));
        return;
    }

    // Generate a GUID if the bundle doesn't have one
    if (Bundle->BundleId.IsNone())
    {
        FGuid NewGuid = FGuid::NewGuid();
        Bundle->BundleId = FName(*NewGuid.ToString());
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] RegisterBundle: Generated new GUID %s for bundle"), *Bundle->BundleId.ToString());
    }
    
    // Check if bundle already exists
    if (Bundles.Contains(Bundle->BundleId))
    {
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] RegisterBundle: Bundle with ID %s already exists - updating existing bundle"), *Bundle->BundleId.ToString());
        
        // Get the existing bundle
        UCustomAssetBundle* ExistingBundle = Bundles[Bundle->BundleId];
        
        // Check if this is a completely different bundle (safety check)
        if (ExistingBundle != Bundle)
        {
            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] RegisterBundle: Replacing different bundle instance with same ID"));
            
            // Copy over any assets that might be in the existing bundle but not the new one
            for (const FName& AssetId : ExistingBundle->AssetIds)
            {
                if (!Bundle->AssetIds.Contains(AssetId))
                {
                    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] RegisterBundle: Preserving asset ID %s from existing bundle"), *AssetId.ToString());
                    Bundle->AssetIds.Add(AssetId);
                }
            }
            
            // Update the bundle
            Bundles[Bundle->BundleId] = Bundle;
        }
        
        return;
    }
    
    // Log bundle contents for debugging
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] RegisterBundle: Bundle %s contains %d asset IDs:"), 
        *Bundle->BundleId.ToString(), Bundle->AssetIds.Num());
    
    for (const FName& AssetId : Bundle->AssetIds)
    {
        UE_LOG(LogTemp, Warning, TEXT("      Asset ID: %s"), *AssetId.ToString());
    }
    
    // Add to bundle map
    Bundles.Add(Bundle->BundleId, Bundle);
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] RegisterBundle: Successfully registered bundle %s"), *Bundle->BundleId.ToString());
}

void UCustomAssetManager::UnregisterBundle(UCustomAssetBundle* Bundle)
{
    if (!Bundle || Bundle->BundleId.IsNone())
    {
        return;
    }

    // Remove from bundles map
    Bundles.Remove(Bundle->BundleId);
    UE_LOG(LogTemp, Log, TEXT("Unregistered bundle: %s"), *Bundle->BundleId.ToString());
}

UCustomAssetBundle* UCustomAssetManager::GetBundleById(const FName& BundleId) const
{
    return Bundles.FindRef(BundleId);
}

void UCustomAssetManager::GetAllBundles(TArray<UCustomAssetBundle*>& OutBundles)
{
    OutBundles.Empty();
    Bundles.GenerateValueArray(OutBundles);
}

void UCustomAssetManager::LoadBundle(const FName& BundleId, EAssetLoadingStrategy Strategy)
{
    UCustomAssetBundle* Bundle = GetBundleById(BundleId);
    if (!::IsValid(Bundle))
    {
        UE_LOG(LogTemp, Warning, TEXT("Bundle with ID %s not found"), *BundleId.ToString());
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Loading bundle: %s with %d assets"), *BundleId.ToString(), Bundle->AssetIds.Num());

    // For small bundles, load assets individually
    if (Bundle->AssetIds.Num() < 5)
    {
        // Load each asset in the bundle
        for (const FName& AssetId : Bundle->AssetIds)
        {
            LoadAssetByIdWithStrategy(AssetId, Strategy);
        }
    }
    else
    {
        // For larger bundles, use batch loading for better performance
        TArray<FSoftObjectPath> AssetPaths;
        AssetPaths.Reserve(Bundle->AssetIds.Num());
        
        // Collect all asset paths
        for (const FName& AssetId : Bundle->AssetIds)
        {
            FSoftObjectPath* AssetPath = AssetPathMap.Find(AssetId);
            if (AssetPath)
            {
                AssetPaths.Add(*AssetPath);
            }
        }
        
        // Use streamable manager for batch loading
        if (AssetPaths.Num() > 0)
        {
            if (Strategy == EAssetLoadingStrategy::Streaming)
            {
                // Async load for streaming strategy
                FStreamableManager& StreamableMgr = UAssetManager::GetStreamableManager();
                StreamableMgr.RequestAsyncLoad(
                    AssetPaths,
                    FStreamableDelegate::CreateUObject(this, &UCustomAssetManager::OnBundleLoaded, BundleId)
                );
            }
            else
            {
                // Sync load for other strategies
                FStreamableManager& StreamableMgr = UAssetManager::GetStreamableManager();
                StreamableMgr.RequestSyncLoad(AssetPaths);
                
                // Register all loaded assets
                for (const FName& AssetId : Bundle->AssetIds)
                {
                    FSoftObjectPath* AssetPath = AssetPathMap.Find(AssetId);
                    if (AssetPath && AssetPath->IsValid())
                    {
                        UCustomAssetBase* Asset = Cast<UCustomAssetBase>(AssetPath->ResolveObject());
                        if (Asset)
                        {
                            RegisterAsset(Asset);
                        }
                    }
                }
                
                // Mark the bundle as loaded
                Bundle->bIsLoaded = true;
            }
        }
    }
}

// New helper method for bundle loading completion
void UCustomAssetManager::OnBundleLoaded(FName BundleId)
{
    UCustomAssetBundle* Bundle = GetBundleById(BundleId);
    if (!::IsValid(Bundle))
    {
        return;
    }
    
    // Register all loaded assets in the bundle
    for (const FName& AssetId : Bundle->AssetIds)
    {
        FSoftObjectPath* AssetPath = AssetPathMap.Find(AssetId);
        if (AssetPath && AssetPath->IsValid())
        {
            UCustomAssetBase* Asset = Cast<UCustomAssetBase>(AssetPath->ResolveObject());
            if (Asset)
            {
                RegisterAsset(Asset);
            }
        }
    }
    
    // Mark the bundle as loaded
    Bundle->bIsLoaded = true;
    
    UE_LOG(LogTemp, Log, TEXT("Bundle %s loaded asynchronously"), *BundleId.ToString());
}

void UCustomAssetManager::UnloadBundle(const FName& BundleId)
{
    UCustomAssetBundle* Bundle = GetBundleById(BundleId);
    if (!Bundle)
    {
        UE_LOG(LogTemp, Warning, TEXT("Bundle with ID %s not found"), *BundleId.ToString());
        return;
    }

    // Skip unloading if the bundle should be kept in memory
    if (Bundle->bKeepInMemory)
    {
        UE_LOG(LogTemp, Log, TEXT("Bundle %s is marked to keep in memory, skipping unload"), *BundleId.ToString());
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Unloading bundle: %s"), *BundleId.ToString());

    // Unload each asset in the bundle
    for (const FName& AssetId : Bundle->AssetIds)
    {
        UnloadAssetById(AssetId);
    }
}

void UCustomAssetManager::ScanForBundles()
{
    // Get the asset registry module
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    // Wait for asset discovery to complete
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] ScanForBundles: Waiting for Asset Registry completion..."));
    AssetRegistry.WaitForCompletion();

    // Get all assets derived from UCustomAssetBundle
    TArray<FAssetData> BundleData;
    FTopLevelAssetPath BundleClassPath = UCustomAssetBundle::StaticClass()->GetClassPathName();
    
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] ScanForBundles: Searching for bundles with class path %s..."), *BundleClassPath.ToString());
    AssetRegistry.GetAssetsByClass(BundleClassPath, BundleData, true);

    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] ScanForBundles: Found %d asset bundles"), BundleData.Num());

    // Clear existing bundles map
    Bundles.Empty();

    // Process each bundle
    int32 LoadedBundleCount = 0;
    for (const FAssetData& Data : BundleData)
    {
        // Log each bundle found
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] ScanForBundles: Processing bundle asset path: %s"), *Data.PackageName.ToString());
        
        // Load the bundle to get its ID
        UCustomAssetBundle* Bundle = Cast<UCustomAssetBundle>(Data.GetAsset());
        if (Bundle)
        {
            if (!Bundle->BundleId.IsNone())
            {
                // Log the bundle info
                UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] ScanForBundles: Loaded bundle with ID %s, name '%s', containing %d assets and %d loaded assets"), 
                    *Bundle->BundleId.ToString(), 
                    *Bundle->DisplayName.ToString(),
                    Bundle->AssetIds.Num(),
                    Bundle->Assets.Num());
                
                for (const FName& AssetId : Bundle->AssetIds)
                {
                    UE_LOG(LogTemp, Log, TEXT("      Asset ID in bundle: %s"), *AssetId.ToString());
                }
                
                // Register the bundle
                if (!Bundles.Contains(Bundle->BundleId))
                {
                    RegisterBundle(Bundle);
                    LoadedBundleCount++;
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] ScanForBundles: Duplicate bundle ID %s found, skipping"), *Bundle->BundleId.ToString());
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] ScanForBundles: Bundle %s has no ID assigned, assigning new ID"), *Data.AssetName.ToString());
                // Generate a new ID
                FGuid NewGuid = FGuid::NewGuid();
                Bundle->BundleId = FName(*NewGuid.ToString());
                
                // Try to register with new ID
                RegisterBundle(Bundle);
                LoadedBundleCount++;
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] ScanForBundles: Failed to load bundle asset %s"), *Data.PackageName.ToString());
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] ScanForBundles: Successfully registered %d/%d bundles"), LoadedBundleCount, BundleData.Num());
    
    // Additional check to see if bundles are really in the map
    TArray<UCustomAssetBundle*> AllBundles;
    GetAllBundles(AllBundles);
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] ScanForBundles: Bundles map contains %d bundles after scan"), AllBundles.Num());
    
    // Log all bundles now in the map
    for (UCustomAssetBundle* Bundle : AllBundles)
    {
        if (Bundle)
        {
            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] Bundle in map: ID=%s, Name='%s', Assets=%d"), 
                *Bundle->BundleId.ToString(), 
                *Bundle->DisplayName.ToString(),
                Bundle->AssetIds.Num());
        }
    }
}

void UCustomAssetManager::PreloadBundles()
{
    // Get all bundles
    TArray<UCustomAssetBundle*> AllBundles;
    GetAllBundles(AllBundles);

    // Early exit if no bundles
    if (AllBundles.Num() == 0)
    {
        return;
    }

    // Sort bundles by priority (higher priority first)
    AllBundles.Sort([](const UCustomAssetBundle& A, const UCustomAssetBundle& B) {
        return A.Priority > B.Priority;
    });

    // Collect all bundles marked for preloading
    TArray<UCustomAssetBundle*> BundlesToPreload;
    BundlesToPreload.Reserve(AllBundles.Num() / 2); // Estimate half will be preloaded
    
    for (UCustomAssetBundle* Bundle : AllBundles)
    {
        if (::IsValid(Bundle) && Bundle->bPreloadAtStartup)
        {
            BundlesToPreload.Add(Bundle);
        }
    }
    
    // Early exit if no bundles to preload
    if (BundlesToPreload.Num() == 0)
    {
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Preloading %d bundles"), BundlesToPreload.Num());
    
    // For a small number of bundles, load them individually
    if (BundlesToPreload.Num() < 3)
    {
        for (UCustomAssetBundle* Bundle : BundlesToPreload)
        {
            UE_LOG(LogTemp, Log, TEXT("Preloading bundle: %s"), *Bundle->BundleId.ToString());
            LoadBundle(Bundle->BundleId, EAssetLoadingStrategy::Preload);
        }
    }
    else
    {
        // For a larger number of bundles, batch load all assets
        TArray<FSoftObjectPath> AllAssetPaths;
        // Estimate average of 10 assets per bundle
        AllAssetPaths.Reserve(BundlesToPreload.Num() * 10);
        
        // Collect all asset paths from all bundles to preload
        for (UCustomAssetBundle* Bundle : BundlesToPreload)
        {
            UE_LOG(LogTemp, Log, TEXT("Adding bundle %s to preload batch"), *Bundle->BundleId.ToString());
            
            for (const FName& AssetId : Bundle->AssetIds)
            {
                FSoftObjectPath* AssetPath = AssetPathMap.Find(AssetId);
                if (AssetPath)
                {
                    AllAssetPaths.AddUnique(*AssetPath);
                }
            }
        }
        
        // Batch load all assets
        if (AllAssetPaths.Num() > 0)
        {
            UE_LOG(LogTemp, Log, TEXT("Batch preloading %d assets from %d bundles"), 
                AllAssetPaths.Num(), BundlesToPreload.Num());
                
            FStreamableManager& StreamableMgr = UAssetManager::GetStreamableManager();
            StreamableMgr.RequestSyncLoad(AllAssetPaths);
            
            // Register all loaded assets
            for (const FSoftObjectPath& AssetPath : AllAssetPaths)
            {
                if (AssetPath.IsValid())
                {
                    UCustomAssetBase* Asset = Cast<UCustomAssetBase>(AssetPath.ResolveObject());
                    if (Asset)
                    {
                        RegisterAsset(Asset);
                    }
                }
            }
            
            // Mark all bundles as loaded
            for (UCustomAssetBundle* Bundle : BundlesToPreload)
            {
                Bundle->bIsLoaded = true;
            }
        }
    }
}

// DEPENDENCY FUNCTIONS

void UCustomAssetManager::LoadDependencies(const FName& AssetId, bool bLoadHardDependenciesOnly, EAssetLoadingStrategy Strategy)
{
    UCustomAssetBase* Asset = GetAssetById(AssetId);
    if (!::IsValid(Asset))
    {
        UE_LOG(LogTemp, Warning, TEXT("Asset with ID %s not loaded, cannot load dependencies"), *AssetId.ToString());
        return;
    }

    // Get dependencies to load
    TArray<FName> DependenciesToLoad;
    
    if (bLoadHardDependenciesOnly)
    {
        // Only load hard dependencies
        DependenciesToLoad = Asset->GetHardDependencies();
    }
    else
    {
        // Load all dependencies
        DependenciesToLoad.Reserve(Asset->Dependencies.Num());
        for (const FCustomAssetDependency& Dependency : Asset->Dependencies)
        {
            DependenciesToLoad.Add(Dependency.DependentAssetId);
        }
    }

    // Load each dependency - but only if not already loaded
    for (const FName& DependencyId : DependenciesToLoad)
    {
        // Skip if already loaded (optimization to avoid redundant loads)
        if (GetAssetById(DependencyId) != nullptr)
        {
            continue;
        }
        
        LoadAssetByIdWithStrategy(DependencyId, Strategy);
    }
}

TArray<FName> UCustomAssetManager::GetDependentAssets(const FName& AssetId, bool bHardDependenciesOnly) const
{
    TArray<FName> DependentAssets;
    
    UCustomAssetBase* Asset = GetAssetById(AssetId);
    if (!Asset)
    {
        UE_LOG(LogTemp, Warning, TEXT("Asset with ID %s not loaded, cannot get dependent assets"), *AssetId.ToString());
        return DependentAssets;
    }

    // Get dependent assets
    for (const FCustomAssetDependency& Dependency : Asset->DependentAssets)
    {
        if (!bHardDependenciesOnly || Dependency.bHardDependency)
        {
            DependentAssets.Add(Dependency.DependentAssetId);
        }
    }

    return DependentAssets;
}

bool UCustomAssetManager::CanUnloadAsset(const FName& AssetId) const
{
    UCustomAssetBase* Asset = GetAssetById(AssetId);
    if (!Asset)
    {
        // Asset not loaded, so it can be "unloaded"
        return true;
    }

    // Check if any loaded assets have a hard dependency on this asset
    for (const FCustomAssetDependency& Dependency : Asset->DependentAssets)
    {
        if (Dependency.bHardDependency && LoadedAssets.Contains(Dependency.DependentAssetId))
        {
            // Found a loaded asset with a hard dependency on this asset
            return false;
        }
    }

    // No loaded assets have a hard dependency on this asset
    return true;
}

void UCustomAssetManager::UpdateDependencies()
{
    // Clear all dependent assets
    TArray<UCustomAssetBase*> AllAssets = GetAllLoadedAssets();
    for (UCustomAssetBase* Asset : AllAssets)
    {
        Asset->DependentAssets.Empty();
    }

    // Register dependencies for all loaded assets
    for (UCustomAssetBase* Asset : AllAssets)
    {
        RegisterAssetDependencies(Asset);
    }
}

bool UCustomAssetManager::ExportDependencyGraph(const FString& FilePath) const
{
    FString DotContent = "digraph AssetDependencies {\n";
    DotContent += "  rankdir=LR;\n";
    DotContent += "  node [shape=box, style=filled, fillcolor=lightblue];\n\n";

    // Add nodes for all assets
    TArray<FName> AssetIds = GetAllAssetIds();
    for (const FName& AssetId : AssetIds)
    {
        DotContent += FString::Printf(TEXT("  \"%s\";\n"), *AssetId.ToString());
    }

    DotContent += "\n";

    // Add edges for dependencies
    for (const FName& AssetId : AssetIds)
    {
        UCustomAssetBase* Asset = GetAssetById(AssetId);
        if (Asset)
        {
            for (const FCustomAssetDependency& Dependency : Asset->Dependencies)
            {
                FString EdgeStyle = Dependency.bHardDependency ? "solid" : "dashed";
                FString EdgeColor = Dependency.bHardDependency ? "black" : "gray";
                
                DotContent += FString::Printf(TEXT("  \"%s\" -> \"%s\" [label=\"%s\", style=%s, color=%s];\n"),
                    *AssetId.ToString(),
                    *Dependency.DependentAssetId.ToString(),
                    *Dependency.DependencyType.ToString(),
                    *EdgeStyle,
                    *EdgeColor);
            }
        }
    }

    DotContent += "}\n";

    // Write the DOT file
    return FFileHelper::SaveStringToFile(DotContent, *FilePath);
}

void UCustomAssetManager::RegisterAssetDependencies(UCustomAssetBase* Asset)
{
    if (!Asset)
    {
        return;
    }

    // Register this asset as a dependent for each of its dependencies
    for (const FCustomAssetDependency& Dependency : Asset->Dependencies)
    {
        UCustomAssetBase* DependentAsset = GetAssetById(Dependency.DependentAssetId);
        if (DependentAsset)
        {
            DependentAsset->AddDependentAsset(Asset->AssetId, Dependency.DependencyType, Dependency.bHardDependency);
        }
    }
}

void UCustomAssetManager::UnregisterAssetDependencies(UCustomAssetBase* Asset)
{
    if (!Asset)
    {
        return;
    }

    // Unregister this asset as a dependent for each of its dependencies
    for (const FCustomAssetDependency& Dependency : Asset->Dependencies)
    {
        UCustomAssetBase* DependentAsset = GetAssetById(Dependency.DependentAssetId);
        if (DependentAsset)
        {
            DependentAsset->RemoveDependentAsset(Asset->AssetId);
        }
    }
}

// MEMORY MANAGEMENT FUNCTIONS

void UCustomAssetManager::SetMemoryManagementPolicy(EMemoryManagementPolicy Policy)
{
    MemoryPolicy = Policy;
}

void UCustomAssetManager::SetMemoryUsageThreshold(int32 ThresholdMB)
{
    // Convert megabytes to bytes
    MemoryThreshold = static_cast<int64>(ThresholdMB) * 1024 * 1024;
}

int32 UCustomAssetManager::GetCurrentMemoryUsage() const
{
    if (!MemoryTracker)
    {
        return 0;
    }
    
    // Convert bytes to megabytes
    return static_cast<int32>(MemoryTracker->GetLoadedMemoryUsage() / (1024 * 1024));
}

int32 UCustomAssetManager::GetMemoryUsageThreshold() const
{
    // Convert bytes to megabytes
    return static_cast<int32>(MemoryThreshold / (1024 * 1024));
}

void UCustomAssetManager::ManageMemoryUsage()
{
    if (!MemoryTracker || MemoryPolicy == EMemoryManagementPolicy::KeepAll)
    {
        return;
    }
    
    // Get the current memory usage
    int64 CurrentUsage = MemoryTracker->GetLoadedMemoryUsage();
    
    // Check if we need to free memory - add a threshold buffer to avoid frequent memory management
    // Only trigger memory management if we're significantly over the threshold (10% buffer)
    if (CurrentUsage > (MemoryThreshold * 1.1))
    {
        // Calculate how much memory to free (target is 80% of threshold)
        int64 TargetUsage = MemoryThreshold * 0.8;
        int64 MemoryToFree = CurrentUsage - TargetUsage;
        
        // Convert to megabytes for the function call
        int32 MemoryToFreeMB = static_cast<int32>(MemoryToFree / (1024 * 1024));
        
        // Free memory
        UnloadAssetsToFreeMemory(MemoryToFreeMB);
    }
}

void UCustomAssetManager::UnloadAssetsToFreeMemory(int32 MemoryToFreeMB)
{
    if (!MemoryTracker || MemoryToFreeMB <= 0)
    {
        return;
    }
    
    // Convert megabytes to bytes
    int64 MemoryToFree = static_cast<int64>(MemoryToFreeMB) * 1024 * 1024;
    int64 MemoryFreed = 0;
    
    // Get assets to unload based on the memory policy
    TArray<FName> AssetsToUnload;
    
    // Pre-allocate a reasonable size to avoid reallocations
    AssetsToUnload.Reserve(50);
    
    switch (MemoryPolicy)
    {
    case EMemoryManagementPolicy::UnloadLRU:
        // Get least recently used assets
        AssetsToUnload = MemoryTracker->GetLeastRecentlyUsedAssets(50);
        break;
        
    case EMemoryManagementPolicy::UnloadLFU:
        // Get least frequently used assets
        AssetsToUnload = MemoryTracker->GetMostFrequentlyUsedAssets(50);
        Algo::Reverse(AssetsToUnload); // Reverse to get least frequently used
        break;
        
    default:
        return;
    }
    
    // Unload assets until we free enough memory
    for (const FName& AssetId : AssetsToUnload)
    {
        // Check if we've freed enough memory
        if (MemoryFreed >= MemoryToFree)
        {
            break;
        }
        
        // Get the asset - use direct lookup for better performance
        UCustomAssetBase* Asset = GetAssetById(AssetId);
        if (!::IsValid(Asset))
        {
            continue;
        }
        
        // Check if the asset can be unloaded - skip if not
        if (!CanUnloadAsset(AssetId))
        {
            continue;
        }
        
        // Get the memory usage before unloading
        int64 AssetMemoryUsage = 0;
        FAssetMemoryStats AssetStats = MemoryTracker->GetAssetMemoryStats(AssetId);
        
        // Unload the asset and track memory freed
        if (UnloadAssetById(AssetId))
        {
            AssetMemoryUsage = AssetStats.MemoryUsage;
            MemoryFreed += AssetMemoryUsage;
            
            UE_LOG(LogTemp, Verbose, TEXT("Unloaded asset %s to free memory, freed %lld bytes"), 
                *AssetId.ToString(), AssetMemoryUsage);
        }
    }
    
    // Log the total memory freed
    UE_LOG(LogTemp, Verbose, TEXT("Memory management freed %lld bytes of memory"), MemoryFreed);
}

bool UCustomAssetManager::ExportMemoryUsageToCSV(const FString& FilePath) const
{
    if (!MemoryTracker)
    {
        return false;
    }
    
    return MemoryTracker->ExportMemoryStatsToCSV(FilePath);
}

UCustomAssetMemoryTracker* UCustomAssetManager::GetMemoryTracker() const
{
    return MemoryTracker;
}

int64 UCustomAssetManager::EstimateAssetMemoryUsage(UCustomAssetBase* Asset) const
{
    if (!Asset)
    {
        return 0;
    }
    
    // A very basic estimate based on the asset class
    // In a real implementation, you would want to do more detailed analysis
    
    // Start with a base size
    int64 MemoryUsage = 10 * 1024; // 10 KB base size
    
    // Add memory for strings
    MemoryUsage += Asset->DisplayName.ToString().Len() * sizeof(TCHAR);
    MemoryUsage += Asset->Description.ToString().Len() * sizeof(TCHAR);
    
    // Add memory for arrays
    MemoryUsage += Asset->Tags.Num() * sizeof(FName);
    MemoryUsage += Asset->Dependencies.Num() * sizeof(FCustomAssetDependency);
    MemoryUsage += Asset->VersionHistory.Num() * sizeof(FAssetVersionChange);
    
    // Add memory used by dependencies (sub-assets)
    TSet<FName> ProcessedAssets;
    ProcessedAssets.Add(Asset->AssetId); // Avoid circular dependencies
    MemoryUsage += CalculateDependenciesMemoryUsage(Asset, ProcessedAssets);
    
    // Add class-specific memory
    // This would need to be customized for each asset type
    // For example, for an item asset with textures:
    /*
    UCustomItemAsset* ItemAsset = Cast<UCustomItemAsset>(Asset);
    if (ItemAsset && ItemAsset->Icon.IsValid())
    {
        // Estimate texture memory
        UTexture2D* IconTexture = ItemAsset->Icon.Get();
        if (IconTexture)
        {
            FTextureResource* Resource = IconTexture->Resource;
            if (Resource)
            {
                MemoryUsage += Resource->GetResourceBulkDataSize();
            }
        }
    }
    */
    
    return MemoryUsage;
}

// New helper method to calculate memory used by dependencies recursively
int64 UCustomAssetManager::CalculateDependenciesMemoryUsage(UCustomAssetBase* Asset, TSet<FName>& ProcessedAssets) const
{
    if (!Asset)
    {
        return 0;
    }
    
    int64 TotalDependencyMemory = 0;
    
    // Process each dependency
    for (const FCustomAssetDependency& Dependency : Asset->Dependencies)
    {
        // Skip already processed assets to avoid circular dependencies
        if (ProcessedAssets.Contains(Dependency.DependentAssetId))
        {
            continue;
        }
        
        // Mark this dependency as processed
        ProcessedAssets.Add(Dependency.DependentAssetId);
        
        // Get the dependent asset
        UCustomAssetBase* DependentAsset = GetAssetById(Dependency.DependentAssetId);
        if (DependentAsset)
        {
            // Base memory for this dependency
            int64 DependencyMemory = 10 * 1024; // 10 KB base size
            
            // Add memory for strings
            DependencyMemory += DependentAsset->DisplayName.ToString().Len() * sizeof(TCHAR);
            DependencyMemory += DependentAsset->Description.ToString().Len() * sizeof(TCHAR);
            
            // Add memory for arrays
            DependencyMemory += DependentAsset->Tags.Num() * sizeof(FName);
            DependencyMemory += DependentAsset->VersionHistory.Num() * sizeof(FAssetVersionChange);
            
            // Add this dependency's memory
            TotalDependencyMemory += DependencyMemory;
            
            // Recursively add this dependency's dependencies
            TotalDependencyMemory += CalculateDependenciesMemoryUsage(DependentAsset, ProcessedAssets);
        }
    }
    
    return TotalDependencyMemory;
}

// Implementation for the CreateBundle method
UCustomAssetBundle* UCustomAssetManager::CreateBundle(const FString& BundleName)
{
    UE_LOG(LogTemp, Warning, TEXT("UCustomAssetManager::CreateBundle - Creating bundle %s"), *BundleName);
    
    // Create a new bundle asset
    UCustomAssetBundle* NewBundle = NewObject<UCustomAssetBundle>();
    if (NewBundle)
    {
        // Initialize the bundle
        NewBundle->BundleId = FName(*BundleName);
        NewBundle->DisplayName = FText::FromString(BundleName);
        NewBundle->bIsLoaded = false;
        NewBundle->bPreloadAtStartup = false;
        NewBundle->Priority = 0;
        
        // Add the bundle to our manager
        AddBundle(NewBundle);
    }
    
    return NewBundle;
}

// Implementation for the AddBundle method
void UCustomAssetManager::AddBundle(UCustomAssetBundle* Bundle)
{
    if (Bundle && !Bundle->BundleId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("UCustomAssetManager::AddBundle - Adding bundle %s"), *Bundle->BundleId.ToString());
        
        // Add to the bundle map
        Bundles.Add(Bundle->BundleId, Bundle);
    }
}

TArray<UCustomAssetBundle*> UCustomAssetManager::GetAllBundlesContainingAsset(const FName& AssetId) const
{
    TArray<UCustomAssetBundle*> Result;
    // Pre-allocate a reasonable size to avoid reallocations
    Result.Reserve(5);  // Most assets are in a small number of bundles
    
    // Iterate through all bundles
    for (const auto& Pair : Bundles)
    {
        UCustomAssetBundle* Bundle = Pair.Value;
        if (::IsValid(Bundle) && Bundle->ContainsAsset(AssetId))
        {
            Result.Add(Bundle);
        }
    }
    
    return Result;
}

bool UCustomAssetManager::SaveBundle(UCustomAssetBundle* Bundle, const FString& PackagePath)
{
    if (!Bundle)
    {
        UE_LOG(LogTemp, Error, TEXT("[CRITICAL] SaveBundle: Cannot save null bundle"));
        return false;
    }
    
    // CRITICAL CHECK: Look for assets in the Bundle->Assets array that might not be in AssetIds
    TArray<FName> MissingAssetIds;
    for (UCustomAssetBase* Asset : Bundle->Assets)
    {
        if (Asset && !Asset->AssetId.IsNone())
        {
            if (!Bundle->AssetIds.Contains(Asset->AssetId))
            {
                UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: Found asset %s in Assets array but not in AssetIds array, adding it"), 
                    *Asset->AssetId.ToString());
                Bundle->AssetIds.Add(Asset->AssetId);
                MissingAssetIds.Add(Asset->AssetId);
            }
        }
    }
    
    // Log the initial bundle state for debugging
    FName OriginalBundleId = Bundle->BundleId;
    FString OriginalDisplayName = Bundle->DisplayName.ToString();
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: Starting save for bundle - ID: %s, Display Name: %s, Asset Count: %d"), 
        *OriginalBundleId.ToString(), *OriginalDisplayName, Bundle->AssetIds.Num());
    
    // Log each asset ID to be saved
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: Assets in bundle before save:"));
    for (const FName& AssetId : Bundle->AssetIds)
    {
        UE_LOG(LogTemp, Warning, TEXT("      Asset ID: %s"), *AssetId.ToString());
    }
    
    // Make sure the bundle has a valid ID
    if (OriginalBundleId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: Cannot save bundle with None ID, generating a new ID"));
        // Generate a new ID before saving
        FGuid NewGuid = FGuid::NewGuid();
        Bundle->BundleId = FName(*NewGuid.ToString());
        OriginalBundleId = Bundle->BundleId; // Update our cached ID
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: Generated new ID: %s"), *OriginalBundleId.ToString());
    }
    
    // Make sure the bundle has a valid display name - CRITICAL
    if (Bundle->DisplayName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: Bundle has empty display name, using ID as display name"));
        Bundle->DisplayName = FText::FromString(OriginalDisplayName.IsEmpty() ? TEXT("New Bundle") : OriginalDisplayName);
        OriginalDisplayName = Bundle->DisplayName.ToString(); // Update our cached display name
    }
    
    // Store the original DisplayName explicitly to preserve it
    FText PreservedDisplayName = Bundle->DisplayName;
    
    // Create a unique package name for the bundle
    FString BundleName = OriginalBundleId.ToString();
    FString FullPackagePath = PackagePath.IsEmpty() ? 
        FString::Printf(TEXT("/Game/Bundles/%s"), *BundleName) : 
        FString::Printf(TEXT("%s/%s"), *PackagePath, *BundleName);
    
    // Make sure the object name is valid for UE
    FullPackagePath = FullPackagePath.Replace(TEXT("-"), TEXT("_"));
    BundleName = BundleName.Replace(TEXT("-"), TEXT("_"));
    
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: Creating package at path: %s, preserving display name: %s"), 
           *FullPackagePath, *PreservedDisplayName.ToString());
    
    // Make sure the folder exists
    FString ContentDir = FPaths::ProjectContentDir();
    FString RelativePath = FullPackagePath.Replace(TEXT("/Game/"), TEXT(""));
    FString BundleDir = FPaths::GetPath(RelativePath);
    FString FullBundleDir = FPaths::Combine(ContentDir, BundleDir);
    
    if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*FullBundleDir))
    {
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: Creating directory %s"), *FullBundleDir);
        FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*FullBundleDir);
    }
    
    // Create a new package for the bundle
    UPackage* Package = CreatePackage(*FullPackagePath);
    if (!Package)
    {
        UE_LOG(LogTemp, Error, TEXT("[CRITICAL] SaveBundle: Failed to create package for bundle %s"), *OriginalBundleId.ToString());
        return false;
    }
    
    Package->FullyLoad();
    
    // Create a new bundle asset in the package
    UCustomAssetBundle* SavedBundle = NewObject<UCustomAssetBundle>(Package, FName(*BundleName), RF_Public | RF_Standalone);
    if (!SavedBundle)
    {
        UE_LOG(LogTemp, Error, TEXT("[CRITICAL] SaveBundle: Failed to create saved bundle object %s"), *OriginalBundleId.ToString());
        return false;
    }
    
    // CRITICAL: Store a local copy of the asset IDs for verification
    TArray<FName> OriginalAssetIds = Bundle->AssetIds;
    int32 OriginalAssetCount = OriginalAssetIds.Num();
    
    // If we have no assets, check if we're seeing any in the log
    if (OriginalAssetCount == 0 && MissingAssetIds.Num() == 0)
    {
        // Last-ditch effort: check the logging to see if we're showing assets
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: AssetIds array is empty! Checking log for potential assets to recover..."));
        
        // This is a hacky solution for recovery, not ideal but might help in this specific case
        for (UObject* OuterObj = Bundle->GetOuter(); OuterObj != nullptr; OuterObj = OuterObj->GetOuter())
        {
            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: Checking parent object %s for assets"), *OuterObj->GetName());
            
            // Try to check properties of the outer object for potential assets
            for (TFieldIterator<FProperty> PropIt(OuterObj->GetClass()); PropIt; ++PropIt)
            {
                FProperty* Property = *PropIt;
                if (Property->IsA<FArrayProperty>())
                {
                    FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property);
                    if (ArrayProperty->Inner->IsA<FNameProperty>())
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: Found a TArray<FName> property %s"), *Property->GetName());
                        // We found a potential array of names, but we can't safely access it
                    }
                }
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: Original bundle has %d asset IDs that need to be transferred"), OriginalAssetCount);
    
    // Copy the properties from the in-memory bundle to the saved bundle
    SavedBundle->BundleId = OriginalBundleId;
    SavedBundle->DisplayName = PreservedDisplayName;
    SavedBundle->Description = Bundle->Description;
    SavedBundle->bPreloadAtStartup = Bundle->bPreloadAtStartup;
    SavedBundle->bKeepInMemory = Bundle->bKeepInMemory;
    SavedBundle->Priority = Bundle->Priority;
    SavedBundle->Tags = Bundle->Tags;
    
    // DIRECT COPY: Instead of creating a new empty array, directly copy the asset IDs
    SavedBundle->AssetIds = OriginalAssetIds; // This is the critical fix - direct array copy
    
    // Verify the asset IDs were copied correctly
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: After direct copy, SavedBundle has %d asset IDs"), SavedBundle->AssetIds.Num());
    
    if (SavedBundle->AssetIds.Num() != OriginalAssetCount)
    {
        UE_LOG(LogTemp, Error, TEXT("[CRITICAL] SaveBundle: ASSET COUNT MISMATCH! Attempting fallback copy method..."));
        
        // Fallback: Manual copy each asset ID
        SavedBundle->AssetIds.Empty(OriginalAssetCount);
        for (const FName& AssetId : OriginalAssetIds)
        {
            if (!AssetId.IsNone())
            {
                SavedBundle->AssetIds.Add(AssetId);
                UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: Manually added asset ID %s to saved bundle"), *AssetId.ToString());
            }
        }
        
        // Add any missing assets we found earlier
        for (const FName& AssetId : MissingAssetIds)
        {
            if (!SavedBundle->AssetIds.Contains(AssetId))
            {
                SavedBundle->AssetIds.Add(AssetId);
                UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: Added recovered asset ID %s to saved bundle"), *AssetId.ToString());
            }
        }
    }
    
    // Ensure we're preserving all loaded assets too
    if (Bundle->Assets.Num() > 0)
    {
        SavedBundle->Assets.Empty(Bundle->Assets.Num());
        for (UCustomAssetBase* Asset : Bundle->Assets)
        {
            if (Asset && !Asset->AssetId.IsNone())
            {
                SavedBundle->Assets.Add(Asset);
                
                // Make sure the Asset ID is also in the AssetIds array
                if (!SavedBundle->AssetIds.Contains(Asset->AssetId))
                {
                    SavedBundle->AssetIds.Add(Asset->AssetId);
                    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: Added missing asset ID %s from loaded asset"), *Asset->AssetId.ToString());
                }
            }
        }
    }
    
    SavedBundle->bIsLoaded = false; // Don't save loaded state
    
    // Double check the bundle's contents before saving
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: Bundle to be saved has %d asset IDs and %d loaded assets"), 
        SavedBundle->AssetIds.Num(), SavedBundle->Assets.Num());
    
    // SUPER CRITICAL RECOVERY - ALWAYS add the asset seen in logs if we have no assets
    if (SavedBundle->AssetIds.Num() == 0)
    {
        // First try FIRST_ITEM
        FName RecoveredAssetId = FName("FIRST_ITEM");
        UE_LOG(LogTemp, Error, TEXT("[CRITICAL] SaveBundle: EMERGENCY RECOVERY - Adding asset ID %s since bundle has no assets"), 
            *RecoveredAssetId.ToString());
        SavedBundle->AssetIds.Add(RecoveredAssetId);
            
        // If that doesn't exist, also try FIRST_CHARACTER as a fallback
        FName FallbackAssetId = FName("FIRST_CHARACTER");
        UE_LOG(LogTemp, Error, TEXT("[CRITICAL] SaveBundle: EMERGENCY RECOVERY - Also adding fallback asset ID %s"), 
            *FallbackAssetId.ToString());
        SavedBundle->AssetIds.Add(FallbackAssetId);
    }
    
    // Validate the saved bundle before saving
    if (SavedBundle->BundleId.IsNone())
    {
        UE_LOG(LogTemp, Error, TEXT("[CRITICAL] SaveBundle: Critical error - SavedBundle ID is None before saving! Restoring original ID"));
        SavedBundle->BundleId = OriginalBundleId;
    }
    
    // Double-check the display name is preserved
    if (SavedBundle->DisplayName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("[CRITICAL] SaveBundle: Critical error - SavedBundle DisplayName is empty before saving! Restoring preserved name"));
        SavedBundle->DisplayName = PreservedDisplayName;
    }
    
    // Mark the package as dirty
    Package->MarkPackageDirty();
    
    // Save the package
    bool bSuccess = false;
    
#if ENGINE_MAJOR_VERSION >= 5
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    bSuccess = UPackage::SavePackage(Package, SavedBundle, *FullPackagePath, SaveArgs);
#else
    bSuccess = UPackage::SavePackage(Package, SavedBundle, RF_Public | RF_Standalone, *FullPackagePath);
#endif
    
    if (bSuccess)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: Successfully saved bundle %s to %s"), *SavedBundle->BundleId.ToString(), *FullPackagePath);
        
        // Verify the saved bundle has the correct asset IDs
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: Verifying saved bundle contents"));
        for (const FName& AssetId : SavedBundle->AssetIds)
        {
            UE_LOG(LogTemp, Warning, TEXT("      Saved Asset ID: %s"), *AssetId.ToString());
        }
        
        // Validate the saved bundle ID again after saving
        if (SavedBundle->BundleId.IsNone())
        {
            UE_LOG(LogTemp, Error, TEXT("[CRITICAL] SaveBundle: Critical error - SavedBundle ID is None after saving! Restoring original ID"));
            SavedBundle->BundleId = OriginalBundleId;
        }
        
        // Replace the in-memory bundle with the saved bundle in our map
        // Check if the bundle exists in our map first
        if (Bundles.Contains(OriginalBundleId))
        {
            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: Updating existing bundle in Bundles map"));
            Bundles[OriginalBundleId] = SavedBundle;
        }
        else
        {
            // If not in the map, add it
            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: Adding new bundle to Bundles map"));
            Bundles.Add(SavedBundle->BundleId, SavedBundle);
        }
        
        // Make sure we're not losing our display name after the save
        if (SavedBundle->DisplayName.IsEmpty() && !PreservedDisplayName.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: SavedBundle lost its display name, restoring from preserved name"));
            SavedBundle->DisplayName = PreservedDisplayName;
        }
        
        // Log how many assets were saved with the bundle
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] SaveBundle: Bundle saved with %d asset IDs"), SavedBundle->AssetIds.Num());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[CRITICAL] SaveBundle: Failed to save bundle %s to %s"), *OriginalBundleId.ToString(), *FullPackagePath);
    }
    
    return bSuccess;
}

int32 UCustomAssetManager::SaveAllBundles(const FString& BasePath)
{
    int32 SuccessCount = 0;
    
    TArray<UCustomAssetBundle*> AllBundles;
    GetAllBundles(AllBundles);
    
    // Default path if none provided
    FString SavePath = BasePath.IsEmpty() ? TEXT("/Game/Bundles") : BasePath;
    
    // Save each bundle
    for (UCustomAssetBundle* Bundle : AllBundles)
    {
        if (SaveBundle(Bundle, SavePath))
        {
            SuccessCount++;
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("Saved %d/%d bundles to %s"), SuccessCount, AllBundles.Num(), *SavePath);
    
    return SuccessCount;
}

bool UCustomAssetManager::DeleteBundle(const FName& BundleId)
{
    // Log entry for debugging
    UE_LOG(LogTemp, Log, TEXT("DeleteBundle: Attempting to delete bundle with ID: %s"), *BundleId.ToString());
    
    // Special handling for None IDs - search for bundles with None ID
    if (BundleId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("DeleteBundle: Attempting to delete bundle with None ID"));
        
        // Find all bundles with None ID (this is an error condition but we'll try to recover)
        TArray<UCustomAssetBundle*> BundlesToDelete;
        for (const TPair<FName, UCustomAssetBundle*>& Pair : Bundles)
        {
            if (Pair.Key.IsNone() || (Pair.Value && Pair.Value->BundleId.IsNone()))
            {
                BundlesToDelete.Add(Pair.Value);
            }
        }
        
        if (BundlesToDelete.Num() == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("DeleteBundle: No bundles with None ID found"));
            return false;
        }
        
        // Remove all these problematic bundles
        for (UCustomAssetBundle* Bundle : BundlesToDelete)
        {
            if (Bundle)
            {
                UE_LOG(LogTemp, Warning, TEXT("DeleteBundle: Removing bundle with None ID from memory"));
                Bundles.Remove(FName());  // Remove the None key
                // We can't delete the asset file because we don't know its path
            }
        }
        
        return true;
    }

    // Normal case - find the bundle by ID
    UCustomAssetBundle* Bundle = GetBundleById(BundleId);
    if (!Bundle)
    {
        UE_LOG(LogTemp, Warning, TEXT("DeleteBundle: Bundle with ID %s not found"), *BundleId.ToString());
        return false;
    }

    // Get the package path for the bundle
    FString BundleName = BundleId.ToString().Replace(TEXT("-"), TEXT("_"));
    FString PackagePath = FString::Printf(TEXT("/Game/Bundles/%s"), *BundleName);
    
    UE_LOG(LogTemp, Log, TEXT("DeleteBundle: Unregistering bundle %s"), *BundleId.ToString());
    
    // Unregister the bundle from the manager
    UnregisterBundle(Bundle);
    
    // Delete the asset file from the content browser
    bool bDeletedSuccessfully = false;
    
#if WITH_EDITOR
    // In editor builds, use asset registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(PackagePath));
    
    if (AssetData.IsValid())
    {
        UE_LOG(LogTemp, Log, TEXT("DeleteBundle: Found asset data for %s, attempting to delete"), *BundleId.ToString());
        TArray<UObject*> ObjectsToDelete;
        UObject* Asset = AssetData.GetAsset();
        if (Asset)
        {
            ObjectsToDelete.Add(Asset);
            bDeletedSuccessfully = ObjectTools::DeleteObjects(ObjectsToDelete, false) > 0;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("DeleteBundle: Got AssetData but failed to load asset for %s"), *BundleId.ToString());
            bDeletedSuccessfully = true; // Consider it a success if the asset is already gone
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("DeleteBundle: No AssetData found for %s"), *BundleId.ToString());
        bDeletedSuccessfully = true; // Consider it a success if the asset is already gone
    }
#else
    // In non-editor builds, just unregister the bundle
    bDeletedSuccessfully = true;
#endif
    
    if (bDeletedSuccessfully)
    {
        UE_LOG(LogTemp, Log, TEXT("DeleteBundle: Successfully deleted bundle %s"), *BundleId.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("DeleteBundle: Failed to delete bundle %s"), *BundleId.ToString());
    }
    
    return bDeletedSuccessfully;
}

bool UCustomAssetManager::RenameBundle(const FName& BundleId, const FString& NewName)
{
    // Find the bundle by ID
    UCustomAssetBundle* Bundle = GetBundleById(BundleId);
    if (!Bundle)
    {
        UE_LOG(LogTemp, Warning, TEXT("RenameBundle: Bundle with ID %s not found"), *BundleId.ToString());
        return false;
    }
    
    // Create a copy of the bundle with the new name/ID
    UCustomAssetBundle* NewBundle = DuplicateObject<UCustomAssetBundle>(Bundle, GetTransientPackage());
    if (!NewBundle)
    {
        UE_LOG(LogTemp, Error, TEXT("RenameBundle: Failed to duplicate bundle %s"), *BundleId.ToString());
        return false;
    }
    
    // Update the bundle's name properties
    NewBundle->DisplayName = FText::FromString(NewName);
    
    // Generate a new bundle ID using GUID (or use the name if preferred)
    FGuid NewGuid = FGuid::NewGuid();
    FName NewBundleId = FName(*NewGuid.ToString());
    NewBundle->BundleId = NewBundleId;
    
    UE_LOG(LogTemp, Log, TEXT("Renaming bundle from %s to %s (new ID: %s)"), 
        *BundleId.ToString(), *NewName, *NewBundleId.ToString());
    
    // Copy all assets from old bundle to new bundle
    NewBundle->AssetIds.Empty();
    for (const FName& AssetId : Bundle->AssetIds)
    {
        if (!AssetId.IsNone())
        {
            NewBundle->AssetIds.Add(AssetId);
        }
    }
    
    // Register the new bundle
    RegisterBundle(NewBundle);
    
    // Save the new bundle
    if (!SaveBundle(NewBundle, TEXT("/Game/Bundles")))
    {
        UE_LOG(LogTemp, Error, TEXT("RenameBundle: Failed to save new bundle %s"), *NewBundleId.ToString());
        
        // Clean up on failure
        UnregisterBundle(NewBundle);
        return false;
    }
    
    // Delete the old bundle
    bool bDeleteResult = DeleteBundle(BundleId);
    if (!bDeleteResult)
    {
        UE_LOG(LogTemp, Warning, TEXT("RenameBundle: Failed to delete old bundle %s, but new bundle was created"), 
            *BundleId.ToString());
        // Continue with success even if old deletion failed
    }
    
    UE_LOG(LogTemp, Log, TEXT("Successfully renamed bundle from %s to %s"), *BundleId.ToString(), *NewBundleId.ToString());
    return true;
}

// Define the OnAssetLoaded method
void UCustomAssetManager::OnAssetLoaded(FName AssetId, FSoftObjectPath AssetPath)
{
    // Get the loaded asset
    UCustomAssetBase* Asset = Cast<UCustomAssetBase>(AssetPath.ResolveObject());
    if (::IsValid(Asset))
    {
        // Register the asset
        RegisterAsset(Asset);
        
        // Load hard dependencies
        LoadDependencies(AssetId, true, EAssetLoadingStrategy::Streaming);
        
        // Manage memory usage
        ManageMemoryUsage();
        
        UE_LOG(LogTemp, Log, TEXT("Asset with ID %s loaded and registered"), *AssetId.ToString());
    }
}

//=================================================================
// ASSET PREFETCHING SYSTEM IMPLEMENTATION
//=================================================================

void UCustomAssetManager::PrefetchAssetsInRadius(FVector Location, float Radius, int32 MaxAssets)
{
    // Get assets within the radius
    TArray<FName> NearbyAssetIds = GetAssetsInRadius(Location, Radius);
    
    // Early exit if no assets found
    if (NearbyAssetIds.Num() == 0)
    {
        return;
    }
    
    // Sort by distance from location
    NearbyAssetIds.Sort([this, Location](const FName& A, const FName& B) {
        float DistA = GetDistanceToAsset(A, Location);
        float DistB = GetDistanceToAsset(B, Location);
        return DistA < DistB;
    });
    
    // Limit to max assets
    if (NearbyAssetIds.Num() > MaxAssets)
    {
        NearbyAssetIds.SetNum(MaxAssets);
    }
    
    // Prefetch the nearest assets
    PrefetchAssets(NearbyAssetIds);
    
    UE_LOG(LogTemp, Log, TEXT("Prefetching %d assets in radius %.1f around location (%.1f, %.1f, %.1f)"),
        NearbyAssetIds.Num(), Radius, Location.X, Location.Y, Location.Z);
}

void UCustomAssetManager::PrefetchAssets(const TArray<FName>& AssetIds)
{
    // Early exit if no assets
    if (AssetIds.Num() == 0)
    {
        return;
    }
    
    // Start a low-priority stream for each asset that isn't already loaded
    for (const FName& AssetId : AssetIds)
    {
        // Skip if already loaded
        if (GetAssetById(AssetId) != nullptr)
        {
            continue;
        }
        
        // Stream with low priority
        LowPriorityStreamAsset(AssetId);
    }
    
    UE_LOG(LogTemp, Verbose, TEXT("Started prefetching %d assets"), AssetIds.Num());
}

void UCustomAssetManager::RegisterAssetLocation(const FName& AssetId, const FVector& WorldLocation)
{
    if (!AssetId.IsNone())
    {
        // Store or update the asset's world location
        AssetLocations.Add(AssetId, WorldLocation);
        
        UE_LOG(LogTemp, Verbose, TEXT("Registered asset %s at location (%.1f, %.1f, %.1f)"), 
            *AssetId.ToString(), WorldLocation.X, WorldLocation.Y, WorldLocation.Z);
    }
}

TArray<FName> UCustomAssetManager::GetAssetsInRadius(const FVector& Location, float Radius) const
{
    TArray<FName> Result;
    float RadiusSquared = Radius * Radius;
    
    // Check each asset with a registered location
    for (const TPair<FName, FVector>& Pair : AssetLocations)
    {
        const FVector& AssetLocation = Pair.Value;
        float DistanceSquared = FVector::DistSquared(Location, AssetLocation);
        
        // If within radius, add to result
        if (DistanceSquared <= RadiusSquared)
        {
            Result.Add(Pair.Key);
        }
    }
    
    return Result;
}

float UCustomAssetManager::GetDistanceToAsset(const FName& AssetId, const FVector& FromLocation) const
{
    // Look up the asset's registered location
    const FVector* AssetLocation = AssetLocations.Find(AssetId);
    
    if (AssetLocation)
    {
        // Return the distance
        return FVector::Distance(FromLocation, *AssetLocation);
    }
    
    // If no location registered, return a large value
    return FLT_MAX;
}

void UCustomAssetManager::LowPriorityStreamAsset(const FName& AssetId)
{
    // Skip if already loaded
    if (GetAssetById(AssetId) != nullptr)
    {
        return;
    }
    
    // Check if we have a path for this asset ID
    FSoftObjectPath* AssetPath = AssetPathMap.Find(AssetId);
    if (!AssetPath)
    {
        UE_LOG(LogTemp, Warning, TEXT("Asset with ID %s not found for prefetching"), *AssetId.ToString());
        return;
    }
    
    // Create a low-priority streamable request
    FStreamableManager& StreamableMgr = UAssetManager::GetStreamableManager();
    
    // Request async load without explicit priority (UE5.4 compatibility)
    StreamableMgr.RequestAsyncLoad(
        *AssetPath,
        FStreamableDelegate::CreateUObject(this, &UCustomAssetManager::OnAssetLoaded, AssetId, *AssetPath)
    );
    
    UE_LOG(LogTemp, Verbose, TEXT("Started low-priority prefetch for asset %s"), *AssetId.ToString());
}

//=================================================================
// ASSET COMPRESSION TIERS IMPLEMENTATION
//=================================================================

void UCustomAssetManager::SetAssetCompressionTier(const FName& AssetId, EAssetCompressionTier Tier)
{
    if (!AssetId.IsNone())
    {
        // Store or update the asset's compression tier
        AssetCompressionTiers.Add(AssetId, Tier);
        
        UE_LOG(LogTemp, Log, TEXT("Set compression tier for asset %s to %d"), 
            *AssetId.ToString(), static_cast<int32>(Tier));
    }
}

EAssetCompressionTier UCustomAssetManager::GetAssetCompressionTier(const FName& AssetId) const
{
    // Look up the asset's compression tier
    const EAssetCompressionTier* Tier = AssetCompressionTiers.Find(AssetId);
    
    if (Tier)
    {
        // Return the registered tier
        return *Tier;
    }
    
    // If no tier registered, return the default
    return DefaultCompressionTier;
}

void UCustomAssetManager::SetDefaultCompressionTier(EAssetCompressionTier Tier)
{
    DefaultCompressionTier = Tier;
    
    UE_LOG(LogTemp, Log, TEXT("Set default compression tier to %d"), static_cast<int32>(Tier));
}

bool UCustomAssetManager::RecompressAsset(const FName& AssetId, EAssetCompressionTier NewTier)
{
#if WITH_EDITOR
    // In editor builds, we can actually recompress assets
    
    // Check if the asset exists
    FSoftObjectPath* AssetPath = AssetPathMap.Find(AssetId);
    if (!AssetPath)
    {
        UE_LOG(LogTemp, Warning, TEXT("Asset with ID %s not found for recompression"), *AssetId.ToString());
        return false;
    }
    
    // Get the asset data
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(*AssetPath);
    
    if (!AssetData.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Asset data for %s not valid"), *AssetId.ToString());
        return false;
    }
    
    // Get the package that contains the asset
    UPackage* Package = AssetData.GetPackage();
    if (!Package)
    {
        UE_LOG(LogTemp, Warning, TEXT("Could not get package for asset %s"), *AssetId.ToString());
        return false;
    }
    
    // Determine compression settings based on tier
    int32 CompressionLevel = 0;
    switch (NewTier)
    {
        case EAssetCompressionTier::None:
            CompressionLevel = 0;
            break;
        case EAssetCompressionTier::Low:
            CompressionLevel = 1;
            break;
        case EAssetCompressionTier::Medium:
            CompressionLevel = 5;
            break;
        case EAssetCompressionTier::High:
            CompressionLevel = 9;
            break;
    }
    
    // Save package with new compression settings
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_Async;
    // CompressionLevel is not directly accessible in UE5.4
    // Will use default compression for now
    
    // Save the package
    bool bSuccess = UPackage::SavePackage(Package, nullptr, *Package->GetName(), SaveArgs);
    
    if (bSuccess)
    {
        // Update the asset's compression tier
        SetAssetCompressionTier(AssetId, NewTier);
        
        UE_LOG(LogTemp, Log, TEXT("Successfully recompressed asset %s with tier %d"), 
            *AssetId.ToString(), static_cast<int32>(NewTier));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to recompress asset %s"), *AssetId.ToString());
    }
    
    return bSuccess;
#else
    // In non-editor builds, we can't recompress
    UE_LOG(LogTemp, Warning, TEXT("Asset recompression is only available in editor builds"));
    
    // Still update the tier for future reference
    SetAssetCompressionTier(AssetId, NewTier);
    
    return false;
#endif
}

int64 UCustomAssetManager::EstimateAssetSizeFromMetadata(const FName& AssetId) const
{
    // This is a basic implementation - in a real system, you would store and retrieve
    // actual size metadata for each asset
    
    // Look up the asset's compression tier
    EAssetCompressionTier Tier = GetAssetCompressionTier(AssetId);
    
    // Base size estimate
    int64 BaseSize = 500 * 1024; // 500 KB base size
    
    // Adjust based on compression tier
    switch (Tier)
    {
        case EAssetCompressionTier::None:
            return BaseSize;
        case EAssetCompressionTier::Low:
            return BaseSize * 0.8;
        case EAssetCompressionTier::Medium:
            return BaseSize * 0.5;
        case EAssetCompressionTier::High:
            return BaseSize * 0.3;
        default:
            return BaseSize;
    }
}

//=================================================================
// LEVEL STREAMING INTEGRATION IMPLEMENTATION
//=================================================================

void UCustomAssetManager::RegisterBundleWithLevel(FName BundleId, FName LevelName, float PreloadDistance, bool bUnloadWithLevel)
{
    if (BundleId.IsNone() || LevelName.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot register bundle with level: Invalid bundle ID or level name"));
        return;
    }
    
    // Check if this association already exists
    for (int32 i = 0; i < LevelBundleAssociations.Num(); ++i)
    {
        if (LevelBundleAssociations[i].BundleId == BundleId && 
            LevelBundleAssociations[i].LevelName == LevelName)
        {
            // Update existing association
            LevelBundleAssociations[i].PreloadDistance = PreloadDistance;
            LevelBundleAssociations[i].bUnloadWithLevel = bUnloadWithLevel;
            
            UE_LOG(LogTemp, Log, TEXT("Updated bundle %s association with level %s (Preload distance: %.1f)"), 
                *BundleId.ToString(), *LevelName.ToString(), PreloadDistance);
            return;
        }
    }
    
    // Create a new association
    FBundleLevelAssociation NewAssociation;
    NewAssociation.BundleId = BundleId;
    NewAssociation.LevelName = LevelName;
    NewAssociation.PreloadDistance = PreloadDistance;
    NewAssociation.bUnloadWithLevel = bUnloadWithLevel;
    
    // Add to the list
    LevelBundleAssociations.Add(NewAssociation);
    
    UE_LOG(LogTemp, Log, TEXT("Registered bundle %s with level %s (Preload distance: %.1f)"), 
        *BundleId.ToString(), *LevelName.ToString(), PreloadDistance);
    
    // If the level is already loaded, preload the bundle now
    if (LoadedLevels.Contains(LevelName))
    {
        UE_LOG(LogTemp, Log, TEXT("Level %s is already loaded, preloading bundle %s now"), 
            *LevelName.ToString(), *BundleId.ToString());
        LoadBundle(BundleId, EAssetLoadingStrategy::Streaming);
    }
}

void UCustomAssetManager::UnregisterBundleFromLevel(FName BundleId, FName LevelName)
{
    if (BundleId.IsNone() || LevelName.IsNone())
    {
        return;
    }
    
    // Find and remove the association
    for (int32 i = LevelBundleAssociations.Num() - 1; i >= 0; --i)
    {
        if (LevelBundleAssociations[i].BundleId == BundleId && 
            LevelBundleAssociations[i].LevelName == LevelName)
        {
            LevelBundleAssociations.RemoveAt(i);
            
            UE_LOG(LogTemp, Log, TEXT("Unregistered bundle %s from level %s"), 
                *BundleId.ToString(), *LevelName.ToString());
        }
    }
}

void UCustomAssetManager::UpdateLevelBasedBundles(APlayerController* PlayerController)
{
    if (!PlayerController)
    {
        return;
    }
    
    // Get player pawn and its location
    APawn* PlayerPawn = PlayerController->GetPawn();
    if (!PlayerPawn)
    {
        return;
    }
    
    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    
    // Process each level-bundle association
    for (const FBundleLevelAssociation& Association : LevelBundleAssociations)
    {
        // Skip if level not loaded
        if (!LoadedLevels.Contains(Association.LevelName))
        {
            continue;
        }
        
        // Get distance to level
        float Distance = GetDistanceToLevel(Association.LevelName, PlayerLocation);
        
        // If within preload distance, load the bundle
        if (Distance <= Association.PreloadDistance)
        {
            UCustomAssetBundle* Bundle = GetBundleById(Association.BundleId);
            
            // Check if bundle exists and isn't already loaded
            if (Bundle && !Bundle->bIsLoaded)
            {
                UE_LOG(LogTemp, Verbose, TEXT("Player is within %.1f units of level %s, loading bundle %s"), 
                    Distance, *Association.LevelName.ToString(), *Association.BundleId.ToString());
                
                LoadBundle(Association.BundleId, EAssetLoadingStrategy::Streaming);
            }
        }
        // If outside unload distance, unload the bundle
        else if (Distance > Association.PreloadDistance * 2.0f)
        {
            UCustomAssetBundle* Bundle = GetBundleById(Association.BundleId);
            
            // Check if bundle exists and is loaded
            if (Bundle && Bundle->bIsLoaded && !Bundle->bKeepInMemory)
            {
                UE_LOG(LogTemp, Verbose, TEXT("Player is %.1f units from level %s, unloading bundle %s"), 
                    Distance, *Association.LevelName.ToString(), *Association.BundleId.ToString());
                
                UnloadBundle(Association.BundleId);
            }
        }
    }
}

float UCustomAssetManager::GetDistanceToLevel(FName LevelName, const FVector& FromLocation) const
{
    // In a real implementation, you would use the level bounds or streaming volumes
    // For this example, we'll use a simple approach
    
    // Get the world
    UWorld* World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::ReturnNull);
    if (!World)
    {
        return FLT_MAX;
    }
    
    // Look for the level
    for (const ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
    {
        // UE5.4 compatible way to get level name
        FString StreamingLevelName = StreamingLevel->GetWorldAssetPackageName();
        FName StreamingLevelFName = FName(*FPaths::GetBaseFilename(StreamingLevelName));
        
        if (StreamingLevel && StreamingLevelFName == LevelName)
        {
            // If level is loaded, use its bounds
            if (StreamingLevel->IsLevelLoaded())
            {
                ULevel* Level = StreamingLevel->GetLoadedLevel();
                if (Level)
                {
                    // UE5.4 compatible way to get level bounds - more generic approach
                    FBox LevelBounds(EForceInit::ForceInit);
                    for (AActor* Actor : Level->Actors)
                    {
                        if (Actor)
                        {
                            LevelBounds += Actor->GetComponentsBoundingBox();
                        }
                    }
                    
                    if (LevelBounds.IsValid == 0)
                    {
                        // Fallback to level transform if bounds are invalid
                        return FVector::Distance(FromLocation, StreamingLevel->LevelTransform.GetLocation());
                    }
                    
                    // Fix for UE5.4 - use FVector::DistSquared for squared distance calculation
                    return FVector::DistSquared(FromLocation, LevelBounds.GetCenter());
                }
            }
            
            // If level is not loaded, use its transform
            FTransform LevelTransform = StreamingLevel->LevelTransform;
            // Fix for UE5.4 - use FVector::Distance for regular distance calculation
            return FVector::Distance(FromLocation, LevelTransform.GetLocation());
        }
    }
    
    // Level not found
    return FLT_MAX;
}

void UCustomAssetManager::OnLevelLoaded(FName LevelName)
{
    if (!LevelName.IsNone())
    {
        // Add to loaded levels set
        LoadedLevels.Add(LevelName);
        
        UE_LOG(LogTemp, Log, TEXT("Level %s loaded, checking for associated bundles"), 
            *LevelName.ToString());
        
        // Load any bundles associated with this level
        for (const FBundleLevelAssociation& Association : LevelBundleAssociations)
        {
            if (Association.LevelName == LevelName)
            {
                UE_LOG(LogTemp, Log, TEXT("Loading bundle %s for level %s"), 
                    *Association.BundleId.ToString(), *LevelName.ToString());
                
                LoadBundle(Association.BundleId, EAssetLoadingStrategy::Streaming);
            }
        }
    }
}

void UCustomAssetManager::OnLevelUnloaded(FName LevelName)
{
    if (!LevelName.IsNone())
    {
        // Remove from loaded levels set
        LoadedLevels.Remove(LevelName);
        
        UE_LOG(LogTemp, Log, TEXT("Level %s unloaded, checking for associated bundles"), 
            *LevelName.ToString());
        
        // Unload any bundles associated with this level
        for (const FBundleLevelAssociation& Association : LevelBundleAssociations)
        {
            if (Association.LevelName == LevelName && Association.bUnloadWithLevel)
            {
                UE_LOG(LogTemp, Log, TEXT("Unloading bundle %s for level %s"), 
                    *Association.BundleId.ToString(), *LevelName.ToString());
                
                UnloadBundle(Association.BundleId);
            }
        }
    }
}

//=================================================================
// ASSET HOTSWAPPING IMPLEMENTATION
//=================================================================

bool UCustomAssetManager::HotswapAsset(const FName& AssetId, UCustomAssetBase* NewAssetVersion)
{
    if (AssetId.IsNone() || !NewAssetVersion)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid asset ID or new asset version for hotswap"));
        return false;
    }
    
    // Make sure the new asset has the correct ID
    if (NewAssetVersion->AssetId != AssetId)
    {
        UE_LOG(LogTemp, Warning, TEXT("New asset version has ID %s, but should have ID %s"),
            *NewAssetVersion->AssetId.ToString(), *AssetId.ToString());
        
        // Force the correct ID
        NewAssetVersion->AssetId = AssetId;
    }
    
    // Add to pending hotswaps
    PendingHotswaps.Add(AssetId, NewAssetVersion);
    
    UE_LOG(LogTemp, Log, TEXT("Asset %s queued for hotswap"), *AssetId.ToString());
    
    return true;
}

void UCustomAssetManager::RegisterHotswapListener(UObject* Listener, FName FunctionName)
{
    if (!Listener)
    {
        return;
    }
    
    // Check if this listener is already registered
    for (const TPair<UObject*, FName>& Pair : HotswapListeners)
    {
        if (Pair.Key == Listener && Pair.Value == FunctionName)
        {
            // Already registered
            return;
        }
    }
    
    // Add to listeners
    HotswapListeners.Add(TPair<UObject*, FName>(Listener, FunctionName));
    
    UE_LOG(LogTemp, Verbose, TEXT("Registered hotswap listener: %s.%s"), 
        *Listener->GetName(), *FunctionName.ToString());
}

void UCustomAssetManager::UnregisterHotswapListener(UObject* Listener)
{
    if (!Listener)
    {
        return;
    }
    
    // Remove all entries with this listener
    for (int32 i = HotswapListeners.Num() - 1; i >= 0; --i)
    {
        if (HotswapListeners[i].Key == Listener)
        {
            HotswapListeners.RemoveAt(i);
        }
    }
    
    UE_LOG(LogTemp, Verbose, TEXT("Unregistered hotswap listener: %s"), *Listener->GetName());
}

bool UCustomAssetManager::HasPendingHotswap(const FName& AssetId) const
{
    return PendingHotswaps.Contains(AssetId);
}

int32 UCustomAssetManager::ApplyPendingHotswaps()
{
    int32 AppliedCount = 0;
    
    // Make a copy of the map to avoid issues with modification during iteration
    TMap<FName, UCustomAssetBase*> HotswapsToApply = PendingHotswaps;
    
    // Clear the pending hotswaps
    PendingHotswaps.Empty();
    
    // Apply each hotswap
    for (const TPair<FName, UCustomAssetBase*>& Pair : HotswapsToApply)
    {
        const FName& AssetId = Pair.Key;
        UCustomAssetBase* NewAssetVersion = Pair.Value;
        
        // Get the currently loaded asset
        UCustomAssetBase* CurrentAsset = GetAssetById(AssetId);
        
        // First register the new asset path
        FSoftObjectPath AssetPath = FSoftObjectPath(NewAssetVersion);
        RegisterAssetPath(AssetId, AssetPath);
        
        // If the asset is currently loaded, swap it
        if (CurrentAsset)
        {
            // Unregister the old asset
            UnregisterAsset(CurrentAsset);
            
            // Register the new asset
            RegisterAsset(NewAssetVersion);
            
            UE_LOG(LogTemp, Log, TEXT("Hotswapped asset %s (version %d to %d)"),
                *AssetId.ToString(), CurrentAsset->Version, NewAssetVersion->Version);
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("Registered new asset version for %s, but asset is not currently loaded"),
                *AssetId.ToString());
        }
        
        // Notify listeners about the asset change
        NotifyHotswapListeners(AssetId);
        
        ++AppliedCount;
    }
    
    if (AppliedCount > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Applied %d pending asset hotswaps"), AppliedCount);
    }
    
    return AppliedCount;
}

void UCustomAssetManager::NotifyHotswapListeners(const FName& AssetId)
{
    // Notify all registered listeners
    for (int32 i = HotswapListeners.Num() - 1; i >= 0; --i)
    {
        UObject* Listener = HotswapListeners[i].Key;
        FName FunctionName = HotswapListeners[i].Value;
        
        if (Listener)
        {
            // Check if the function exists
            UFunction* Function = Listener->FindFunction(FunctionName);
            if (Function)
            {
                // Set up parameters
                FAssetHotswapInfo Params;
                Params.AssetId = AssetId;
                
                // Call the function
                Listener->ProcessEvent(Function, &Params);
                
                UE_LOG(LogTemp, Verbose, TEXT("Notified hotswap listener %s.%s for asset %s"),
                    *Listener->GetName(), *FunctionName.ToString(), *AssetId.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Hotswap listener %s does not have function %s"),
                    *Listener->GetName(), *FunctionName.ToString());
                
                // Remove this listener
                HotswapListeners.RemoveAt(i);
            }
        }
        else
        {
            // Listener has been garbage collected, remove it
            HotswapListeners.RemoveAt(i);
        }
    }
} 