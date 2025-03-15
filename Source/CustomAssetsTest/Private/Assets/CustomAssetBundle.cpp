#include "Assets/CustomAssetBundle.h"
#include "Assets/CustomAssetManager.h"

UCustomAssetBundle::UCustomAssetBundle()
{
    // Initialize default values
    Priority = 50;
    bPreloadAtStartup = false;
    bKeepInMemory = false;
    
    // CRITICAL FIX: Explicitly initialize the AssetIds array to ensure it starts empty
    AssetIds.Empty();
    Assets.Empty();
    
    // Mark that the bundle needs to be saved
    bIsLoaded = false;
    
    UE_LOG(LogTemp, Warning, TEXT("UCustomAssetBundle::Constructor - Created new bundle with EMPTY asset arrays"));
}

void UCustomAssetBundle::AddAsset(const FName& AssetId)
{
    if (AssetId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("Attempted to add empty asset ID to bundle %s"), *BundleId.ToString());
        return;
    }

    // Log the operation
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] AddAsset: Adding asset ID %s to bundle %s (DisplayName: %s)"), 
        *AssetId.ToString(), *BundleId.ToString(), *DisplayName.ToString());
    
    // CRITICAL FIX: Make sure AssetIds is directly accessed to ensure property is marked as modified for serialization
    if (AssetIds.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] AddAsset: AssetIds array was empty, initializing it"));
        // Explicitly modify the property (this helps with serialization)
        AssetIds = TArray<FName>();
    }
    
    // Check if the asset ID is already in the bundle
    if (!AssetIds.Contains(AssetId))
    {
        // Add the asset ID to the AssetIds array - directly modify the property
        AssetIds.Add(AssetId);
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] AddAsset: Added asset ID %s to bundle's AssetIds array, now contains %d assets"), 
            *AssetId.ToString(), AssetIds.Num());
        
        // CRITICAL: Mark the object as modified to ensure serialization
        Modify();
        
        // Log all assets in the bundle for debugging
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] AddAsset: Bundle now contains the following assets:"));
        for (const FName& Id : AssetIds)
        {
            UE_LOG(LogTemp, Warning, TEXT("      Asset ID: %s"), *Id.ToString());
        }
        
        // If the asset is loaded, also add it to the Assets array
        UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
        UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
        if (Asset && !Assets.Contains(Asset))
        {
            Assets.Add(Asset);
            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] AddAsset: Asset %s is loaded, also added to Assets array"), *AssetId.ToString());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] AddAsset: Asset %s is not loaded or already in Assets array"), *AssetId.ToString());
        }
    }
    else
    {
        // Even if the ID is already in AssetIds, make sure it's also in the Assets array if loaded
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] AddAsset: Asset ID %s is already in AssetIds array"), *AssetId.ToString());
        
        UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
        UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
        if (Asset && !Assets.Contains(Asset))
        {
            Assets.Add(Asset);
            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] AddAsset: Asset ID %s was already in bundle but asset was loaded, adding to Assets array"), *AssetId.ToString());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] AddAsset: Asset ID %s is already in bundle %s"), *AssetId.ToString(), *BundleId.ToString());
        }
    }
}

void UCustomAssetBundle::RemoveAsset(const FName& AssetId)
{
    if (AssetId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] RemoveAsset: Cannot remove None asset ID from bundle %s"), 
            *BundleId.ToString());
        return;
    }
    
    // Log the operation
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] RemoveAsset: Removing asset ID %s from bundle %s (DisplayName: %s)"), 
        *AssetId.ToString(), *BundleId.ToString(), *DisplayName.ToString());
    
    // CRITICAL FIX: Mark the object as modified before making changes to ensure serialization
    Modify();
    
    // Debug current state before removal
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] RemoveAsset: Before removal - AssetIds count: %d, Assets count: %d"), 
        AssetIds.Num(), Assets.Num());
    
    // CRITICAL FIX: Verify the asset is actually in the bundle before removing
    if (!AssetIds.Contains(AssetId))
    {
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] RemoveAsset: Asset ID %s is not in this bundle's AssetIds array"), 
            *AssetId.ToString());
        
        // Try to check if it might be in the Assets array instead
        UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
        UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
        if (Asset && Assets.Contains(Asset))
        {
            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] RemoveAsset: Asset %s found in Assets array but not in AssetIds, removing it"), 
                *AssetId.ToString());
            Assets.Remove(Asset);
            // No need to continue since the asset ID wasn't in AssetIds
            return;
        }
        
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] RemoveAsset: Asset ID %s not found in either AssetIds or Assets arrays"), 
            *AssetId.ToString());
        return;
    }
    
    // Remove from AssetIds array with verification
    int32 RemovedCount = AssetIds.Remove(AssetId);
    
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] RemoveAsset: Removed %d instances of AssetId %s from AssetIds array"), 
        RemovedCount, *AssetId.ToString());
    
    // Also remove from Assets array if it's loaded
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
    int32 RemovedAssetsCount = 0;
    
    if (Asset)
    {
        RemovedAssetsCount = Assets.Remove(Asset);
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] RemoveAsset: Removed %d instances of loaded Asset from Assets array"), 
            RemovedAssetsCount);
    }
    
    // Debug current state after removal
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] RemoveAsset: After removal - AssetIds count: %d, Assets count: %d"), 
        AssetIds.Num(), Assets.Num());
    
    if (RemovedCount > 0 || RemovedAssetsCount > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] RemoveAsset: Successfully removed asset ID %s from bundle %s"), 
            *AssetId.ToString(), *BundleId.ToString());
            
        // CRITICAL FIX: Save the bundle after removing assets
        Save();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] RemoveAsset: Asset ID %s was not found in bundle %s or failed to remove"), 
            *AssetId.ToString(), *BundleId.ToString());
    }
}

bool UCustomAssetBundle::ContainsAsset(const FName& AssetId) const
{
    if (AssetId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] ContainsAsset: Checking for 'None' AssetId in bundle %s, returning false"), 
            *BundleId.ToString());
        return false;
    }
    
    // Log the state of the arrays
    UE_LOG(LogTemp, Verbose, TEXT("[CRITICAL] ContainsAsset: Checking if bundle %s contains asset %s (AssetIds: %d, Assets: %d)"), 
        *BundleId.ToString(), *AssetId.ToString(), AssetIds.Num(), Assets.Num());
    
    // Always check the AssetIds array first - use direct Contains for better performance
    if (AssetIds.Contains(AssetId))
    {
        UE_LOG(LogTemp, Verbose, TEXT("[CRITICAL] ContainsAsset: Bundle %s contains asset %s in AssetIds array"), 
            *BundleId.ToString(), *AssetId.ToString());
        return true;
    }
    
    // For backward compatibility, also check the loaded Assets array
    // Use a more efficient range-based for loop with a reference to avoid copying
    for (const UCustomAssetBase* Asset : Assets)
    {
        if (IsValid(Asset) && Asset->AssetId == AssetId)
        {
            // CRITICAL FIX: If we find an asset in the Assets array but not in AssetIds, add it to AssetIds
            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] ContainsAsset: Bundle %s contains asset %s in Assets array but not in AssetIds! Adding to AssetIds."), 
                *BundleId.ToString(), *AssetId.ToString());
                
            // Need to cast away constness for this recovery feature
            UCustomAssetBundle* MutableThis = const_cast<UCustomAssetBundle*>(this);
            if (MutableThis)
            {
                MutableThis->AssetIds.AddUnique(AssetId);
                UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] ContainsAsset: Added missing asset ID %s to AssetIds array"), *AssetId.ToString());
            }
            
            return true;
        }
    }
    
    // If we got here, the asset is not in this bundle
    UE_LOG(LogTemp, Verbose, TEXT("[CRITICAL] ContainsAsset: Bundle %s does NOT contain asset %s"), 
        *BundleId.ToString(), *AssetId.ToString());
    
    return false;
}

// Implementation of the Save method
bool UCustomAssetBundle::Save()
{
    if (BundleId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("UCustomAssetBundle::Save - Cannot save bundle with None ID, generating a new ID"));
        // Generate a new ID before saving
        FGuid NewGuid = FGuid::NewGuid();
        BundleId = FName(*NewGuid.ToString());
    }
    
    // Make sure the bundle has a display name
    if (DisplayName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("UCustomAssetBundle::Save - Bundle has no display name, using ID as display name"));
        DisplayName = FText::FromName(BundleId);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("UCustomAssetBundle::Save - Saving bundle %s (DisplayName: %s)"), 
        *BundleId.ToString(), *DisplayName.ToString());
    
    // Use the asset manager's SaveBundle method to save this bundle to the project
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    return AssetManager.SaveBundle(this, TEXT("/Game/Bundles"));
}

void UCustomAssetBundle::DebugPrintContents(const FString& Context) const
{
    FString ContextStr = Context.IsEmpty() ? TEXT("DebugPrintContents") : Context;
    
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] %s: Bundle %s (Display: %s) Contents:"), 
        *ContextStr, *BundleId.ToString(), *DisplayName.ToString());
    
    // Print basic info
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] %s: AssetIds array has %d entries"), *ContextStr, AssetIds.Num());
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] %s: Assets array has %d entries"), *ContextStr, Assets.Num());
    
    // Print all asset IDs
    if (AssetIds.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] %s: AssetIds:"), *ContextStr);
        for (const FName& AssetId : AssetIds)
        {
            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] %s:     Asset ID: %s"), *ContextStr, *AssetId.ToString());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] %s: AssetIds array is EMPTY!"), *ContextStr);
    }
    
    // Print all loaded assets
    if (Assets.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] %s: Loaded Assets:"), *ContextStr);
        for (const UCustomAssetBase* Asset : Assets)
        {
            if (IsValid(Asset))
            {
                UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] %s:     Loaded Asset: %s (ID: %s)"), 
                    *ContextStr, *Asset->GetName(), *Asset->AssetId.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] %s:     Invalid Asset Pointer"), *ContextStr);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] %s: Assets array is EMPTY!"), *ContextStr);
    }
    
    // Log memory address for debugging references
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] %s: Bundle memory address: 0x%p"), *ContextStr, this);
} 