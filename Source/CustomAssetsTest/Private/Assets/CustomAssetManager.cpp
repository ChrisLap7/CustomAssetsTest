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
    // Check if the asset is already loaded
    UCustomAssetBase* LoadedAsset = GetAssetById(AssetId);
    if (LoadedAsset)
    {
        // Record access to the asset
        if (MemoryTracker)
        {
            MemoryTracker->RecordAssetAccess(AssetId);
        }
        
        return LoadedAsset;
    }

    // Check if we have a path for this asset ID
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
        StreamAsset(AssetId, FOnAssetLoaded());
        return nullptr;

    case EAssetLoadingStrategy::LazyLoad:
        // For lazy load, we load synchronously but keep it in memory
        Asset = Cast<UCustomAssetBase>(AssetPath->TryLoad());
        break;

    default:
        // Default to on-demand loading
        Asset = Cast<UCustomAssetBase>(AssetPath->TryLoad());
        break;
    }

    if (Asset)
    {
        // Register the loaded asset
        RegisterAsset(Asset);
        
        // Load hard dependencies
        LoadDependencies(AssetId, true, Strategy);
        
        // Manage memory usage after loading
        ManageMemoryUsage();
        
        return Asset;
    }

    UE_LOG(LogTemp, Warning, TEXT("Failed to load asset with ID %s"), *AssetId.ToString());
    return nullptr;
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
    UCustomAssetBase* Asset = LoadedAssets.FindRef(AssetId);
    
    // Record access to the asset if found
    if (Asset && MemoryTracker)
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
        UE_LOG(LogTemp, Warning, TEXT("UCustomAssetManager::RegisterBundle - Cannot register null bundle"));
        return;
    }

    // Generate a GUID if the bundle doesn't have one
    if (Bundle->BundleId.IsNone())
    {
        FGuid NewGuid = FGuid::NewGuid();
        Bundle->BundleId = FName(*NewGuid.ToString());
        UE_LOG(LogTemp, Warning, TEXT("UCustomAssetManager::RegisterBundle - Generated new GUID %s for bundle"), *Bundle->BundleId.ToString());
    }
    
    // Check if bundle already exists
    if (Bundles.Contains(Bundle->BundleId))
    {
        UE_LOG(LogTemp, Warning, TEXT("UCustomAssetManager::RegisterBundle - Bundle with ID %s already exists"), *Bundle->BundleId.ToString());
        return;
    }
    
    // Add to bundle map
    Bundles.Add(Bundle->BundleId, Bundle);
    UE_LOG(LogTemp, Log, TEXT("UCustomAssetManager::RegisterBundle - Registered bundle %s"), *Bundle->BundleId.ToString());
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
    if (!Bundle)
    {
        UE_LOG(LogTemp, Warning, TEXT("Bundle with ID %s not found"), *BundleId.ToString());
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Loading bundle: %s with %d assets"), *BundleId.ToString(), Bundle->AssetIds.Num());

    // Load each asset in the bundle
    for (const FName& AssetId : Bundle->AssetIds)
    {
        LoadAssetByIdWithStrategy(AssetId, Strategy);
    }
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
    AssetRegistry.WaitForCompletion();

    // Get all assets derived from UCustomAssetBundle
    TArray<FAssetData> BundleData;
    FTopLevelAssetPath BundleClassPath = UCustomAssetBundle::StaticClass()->GetClassPathName();
    AssetRegistry.GetAssetsByClass(BundleClassPath, BundleData, true);

    UE_LOG(LogTemp, Log, TEXT("Found %d asset bundles"), BundleData.Num());

    // Clear existing bundles map
    Bundles.Empty();

    // Process each bundle
    for (const FAssetData& Data : BundleData)
    {
        // Load the bundle to get its ID
        UCustomAssetBundle* Bundle = Cast<UCustomAssetBundle>(Data.GetAsset());
        if (Bundle && !Bundle->BundleId.IsNone())
        {
            // Register the bundle
            RegisterBundle(Bundle);
        }
        else if (Bundle)
        {
            UE_LOG(LogTemp, Warning, TEXT("Bundle %s has no ID assigned"), *Data.AssetName.ToString());
        }
    }
}

void UCustomAssetManager::PreloadBundles()
{
    // Get all bundles
    TArray<UCustomAssetBundle*> AllBundles;
    GetAllBundles(AllBundles);

    // Sort bundles by priority (higher priority first)
    AllBundles.Sort([](const UCustomAssetBundle& A, const UCustomAssetBundle& B) {
        return A.Priority > B.Priority;
    });

    // Preload bundles marked for preloading
    for (UCustomAssetBundle* Bundle : AllBundles)
    {
        if (Bundle->bPreloadAtStartup)
        {
            UE_LOG(LogTemp, Log, TEXT("Preloading bundle: %s"), *Bundle->BundleId.ToString());
            LoadBundle(Bundle->BundleId, EAssetLoadingStrategy::Preload);
        }
    }
}

// DEPENDENCY FUNCTIONS

void UCustomAssetManager::LoadDependencies(const FName& AssetId, bool bLoadHardDependenciesOnly, EAssetLoadingStrategy Strategy)
{
    UCustomAssetBase* Asset = GetAssetById(AssetId);
    if (!Asset)
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
        for (const FCustomAssetDependency& Dependency : Asset->Dependencies)
        {
            DependenciesToLoad.Add(Dependency.DependentAssetId);
        }
    }

    // Load each dependency
    for (const FName& DependencyId : DependenciesToLoad)
    {
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
    
    // Check if we need to free memory
    if (CurrentUsage > MemoryThreshold)
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
        
        // Get the asset
        UCustomAssetBase* Asset = GetAssetById(AssetId);
        if (!Asset)
        {
            continue;
        }
        
        // Check if the asset can be unloaded
        if (!CanUnloadAsset(AssetId))
        {
            continue;
        }
        
        // Get the memory usage before unloading
        int64 AssetMemoryUsage = 0;
        FAssetMemoryStats AssetStats = MemoryTracker->GetAssetMemoryStats(AssetId);
        if (AssetStats.bIsLoaded)
        {
            AssetMemoryUsage = AssetStats.MemoryUsage;
        }
        
        // Unload the asset
        if (UnloadAssetById(AssetId))
        {
            // Update memory freed
            MemoryFreed += AssetMemoryUsage;
            
            UE_LOG(LogTemp, Log, TEXT("Unloaded asset %s to free memory (freed %lld bytes)"), *AssetId.ToString(), AssetMemoryUsage);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("Memory management: freed %lld bytes (target was %lld bytes)"), MemoryFreed, MemoryToFree);
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
    
    if (AssetId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("GetAllBundlesContainingAsset called with empty AssetId"));
        return Result;
    }
    
    // Iterate through all bundles to find ones that contain the asset
    for (const TPair<FName, UCustomAssetBundle*>& BundlePair : Bundles)
    {
        UCustomAssetBundle* Bundle = BundlePair.Value;
        if (!Bundle)
        {
            continue;
        }
        
        // First, check if the asset ID is directly in the bundle's AssetIds array
        if (Bundle->ContainsAsset(AssetId))
        {
            Result.Add(Bundle);
            continue; // Already found the asset in this bundle, no need to check loaded assets
        }
        
        // If not found in AssetIds, check the loaded assets (for backward compatibility)
        // This part is only relevant for assets that are already loaded
        for (UCustomAssetBase* Asset : Bundle->Assets)
        {
            if (Asset && Asset->AssetId == AssetId)
            {
                Result.Add(Bundle);
                break; // Found the asset in this bundle
            }
        }
    }
    
    return Result;
}

bool UCustomAssetManager::SaveBundle(UCustomAssetBundle* Bundle, const FString& PackagePath)
{
    if (!Bundle)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot save null bundle"));
        return false;
    }
    
    // Log the initial bundle state for debugging
    FName OriginalBundleId = Bundle->BundleId;
    FString OriginalDisplayName = Bundle->DisplayName.ToString();
    UE_LOG(LogTemp, Warning, TEXT("SaveBundle: Starting save for bundle - ID: %s, Display Name: %s"), 
        *OriginalBundleId.ToString(), *OriginalDisplayName);
    
    // Make sure the bundle has a valid ID
    if (OriginalBundleId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("SaveBundle: Cannot save bundle with None ID, generating a new ID"));
        // Generate a new ID before saving
        FGuid NewGuid = FGuid::NewGuid();
        Bundle->BundleId = FName(*NewGuid.ToString());
        OriginalBundleId = Bundle->BundleId; // Update our cached ID
        UE_LOG(LogTemp, Warning, TEXT("SaveBundle: Generated new ID: %s"), *OriginalBundleId.ToString());
    }
    
    // Make sure the bundle has a valid display name - CRITICAL
    if (Bundle->DisplayName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("SaveBundle: Bundle has empty display name, using ID as display name"));
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
    
    UE_LOG(LogTemp, Log, TEXT("SaveBundle: Creating package at path: %s, preserving display name: %s"), 
           *FullPackagePath, *PreservedDisplayName.ToString());
    
    // Create a new package for the bundle
    UPackage* Package = CreatePackage(*FullPackagePath);
    if (!Package)
    {
        UE_LOG(LogTemp, Error, TEXT("SaveBundle: Failed to create package for bundle %s"), *OriginalBundleId.ToString());
        return false;
    }
    
    Package->FullyLoad();
    
    // Create a new bundle asset in the package
    UCustomAssetBundle* SavedBundle = NewObject<UCustomAssetBundle>(Package, FName(*BundleName), RF_Public | RF_Standalone);
    if (!SavedBundle)
    {
        UE_LOG(LogTemp, Error, TEXT("SaveBundle: Failed to create saved bundle object %s"), *OriginalBundleId.ToString());
        return false;
    }
    
    // Copy the properties from the in-memory bundle to the saved bundle
    // CRITICAL: Make sure to preserve the bundle ID
    SavedBundle->BundleId = OriginalBundleId;
    UE_LOG(LogTemp, Log, TEXT("SaveBundle: Set SavedBundle ID to: %s"), *SavedBundle->BundleId.ToString());
    
    // CRITICAL: Make sure to preserve the original display name
    SavedBundle->DisplayName = PreservedDisplayName;
    UE_LOG(LogTemp, Log, TEXT("SaveBundle: Set SavedBundle display name to: %s"), *SavedBundle->DisplayName.ToString());
    
    SavedBundle->Description = Bundle->Description;
    SavedBundle->bPreloadAtStartup = Bundle->bPreloadAtStartup;
    SavedBundle->bKeepInMemory = Bundle->bKeepInMemory;
    SavedBundle->Priority = Bundle->Priority;
    
    // Safely copy the AssetIds array
    SavedBundle->AssetIds.Empty(Bundle->AssetIds.Num());
    for (const FName& AssetId : Bundle->AssetIds)
    {
        if (!AssetId.IsNone())
        {
            SavedBundle->AssetIds.Add(AssetId);
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
                }
            }
        }
    }
    
    SavedBundle->bIsLoaded = false; // Don't save loaded state
    
    // Validate the saved bundle before saving
    if (SavedBundle->BundleId.IsNone())
    {
        UE_LOG(LogTemp, Error, TEXT("SaveBundle: Critical error - SavedBundle ID is None before saving! Restoring original ID"));
        SavedBundle->BundleId = OriginalBundleId;
    }
    
    // Double-check the display name is preserved
    if (SavedBundle->DisplayName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("SaveBundle: Critical error - SavedBundle DisplayName is empty before saving! Restoring preserved name"));
        SavedBundle->DisplayName = PreservedDisplayName;
    }
    
    // Mark the package as dirty
    Package->MarkPackageDirty();
    
    // Save the package
    bool bSuccess = false;
    
#if ENGINE_MAJOR_VERSION >= 5
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    bSuccess = UPackage::SavePackage(Package, SavedBundle, *FullPackagePath, SaveArgs);
#else
    bSuccess = UPackage::SavePackage(Package, SavedBundle, RF_Public | RF_Standalone, *FullPackagePath);
#endif
    
    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("SaveBundle: Successfully saved bundle %s to %s"), *SavedBundle->BundleId.ToString(), *FullPackagePath);
        
        // Validate the saved bundle ID again after saving
        if (SavedBundle->BundleId.IsNone())
        {
            UE_LOG(LogTemp, Error, TEXT("SaveBundle: Critical error - SavedBundle ID is None after saving! Restoring original ID"));
            SavedBundle->BundleId = OriginalBundleId;
        }
        
        // Replace the in-memory bundle with the saved bundle in our map
        // Check if the bundle exists in our map first
        if (Bundles.Contains(OriginalBundleId))
        {
            UE_LOG(LogTemp, Log, TEXT("SaveBundle: Updating existing bundle in Bundles map"));
            Bundles[OriginalBundleId] = SavedBundle;
        }
        else
        {
            // If not in the map, add it
            UE_LOG(LogTemp, Log, TEXT("SaveBundle: Adding new bundle to Bundles map"));
            Bundles.Add(SavedBundle->BundleId, SavedBundle);
        }
        
        // Make sure we're not losing our display name after the save
        if (SavedBundle->DisplayName.IsEmpty() && !PreservedDisplayName.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("SaveBundle: SavedBundle lost its display name, restoring from preserved name"));
            SavedBundle->DisplayName = PreservedDisplayName;
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SaveBundle: Failed to save bundle %s to %s"), *OriginalBundleId.ToString(), *FullPackagePath);
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