#include "Assets/CustomAssetBundle.h"
#include "Assets/CustomAssetManager.h"

UCustomAssetBundle::UCustomAssetBundle()
{
    // Initialize default values
    Priority = 50;
    bPreloadAtStartup = false;
    bKeepInMemory = false;
}

void UCustomAssetBundle::AddAsset(const FName& AssetId)
{
    if (AssetId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("Attempted to add empty asset ID to bundle %s"), *BundleId.ToString());
        return;
    }

    // Log the operation
    UE_LOG(LogTemp, Log, TEXT("Adding asset ID %s to bundle %s (DisplayName: %s)"), 
        *AssetId.ToString(), *BundleId.ToString(), *DisplayName.ToString());
    
    // First check if the asset is already in the bundle
    if (!AssetIds.Contains(AssetId))
    {
        // Add the asset ID to the AssetIds array regardless of whether it's loaded
        AssetIds.Add(AssetId);
        UE_LOG(LogTemp, Log, TEXT("Added asset ID %s to bundle's AssetIds array"), *AssetId.ToString());
        
        // If the asset is loaded, also add it to the Assets array
        UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
        UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
        if (Asset && !Assets.Contains(Asset))
        {
            Assets.Add(Asset);
            UE_LOG(LogTemp, Log, TEXT("Asset %s is loaded, also added to Assets array"), *AssetId.ToString());
        }
        else if (!Asset)
        {
            UE_LOG(LogTemp, Log, TEXT("Asset %s is not currently loaded, only added AssetId"), *AssetId.ToString());
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Asset ID %s is already in bundle %s"), *AssetId.ToString(), *BundleId.ToString());
    }
}

void UCustomAssetBundle::RemoveAsset(const FName& AssetId)
{
    if (AssetId.IsNone())
    {
        return;
    }
    
    // Log the operation
    UE_LOG(LogTemp, Log, TEXT("Removing asset ID %s from bundle %s (DisplayName: %s)"), 
        *AssetId.ToString(), *BundleId.ToString(), *DisplayName.ToString());
    
    // Remove from AssetIds array
    bool bRemovedFromIds = AssetIds.Remove(AssetId) > 0;
    
    // Also remove from Assets array if it's loaded
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
    bool bRemovedFromAssets = false;
    if (Asset)
    {
        bRemovedFromAssets = Assets.Remove(Asset) > 0;
    }
    
    if (bRemovedFromIds || bRemovedFromAssets)
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully removed asset ID %s from bundle %s"), *AssetId.ToString(), *BundleId.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Asset ID %s was not found in bundle %s"), *AssetId.ToString(), *BundleId.ToString());
    }
}

bool UCustomAssetBundle::ContainsAsset(const FName& AssetId) const
{
    // Always check the AssetIds array first - use direct Contains for better performance
    if (AssetIds.Contains(AssetId))
    {
        return true;
    }
    
    // For backward compatibility, also check the loaded Assets array
    // Use a more efficient range-based for loop with a reference to avoid copying
    for (const UCustomAssetBase* Asset : Assets)
    {
        if (IsValid(Asset) && Asset->AssetId == AssetId)
        {
            return true;
        }
    }
    
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