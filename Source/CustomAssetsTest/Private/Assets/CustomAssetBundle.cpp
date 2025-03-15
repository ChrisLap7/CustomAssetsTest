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

    if (!AssetIds.Contains(AssetId))
    {
        UE_LOG(LogTemp, Log, TEXT("Adding asset ID %s to bundle %s"), *AssetId.ToString(), *BundleId.ToString());
        AssetIds.Add(AssetId);
        
        // Check if the asset is loaded and add it to the Assets array if it is
        UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
        UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
        if (Asset && !Assets.Contains(Asset))
        {
            Assets.Add(Asset);
        }
    }
}

void UCustomAssetBundle::RemoveAsset(const FName& AssetId)
{
    if (AssetId.IsNone())
    {
        return;
    }
    
    // Remove from AssetIds array
    AssetIds.Remove(AssetId);
    
    // Also remove from Assets array if it's loaded
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
    if (Asset)
    {
        Assets.Remove(Asset);
    }
    
    UE_LOG(LogTemp, Log, TEXT("Removed asset ID %s from bundle %s"), *AssetId.ToString(), *BundleId.ToString());
}

bool UCustomAssetBundle::ContainsAsset(const FName& AssetId) const
{
    return AssetIds.Contains(AssetId);
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
    
    UE_LOG(LogTemp, Warning, TEXT("UCustomAssetBundle::Save - Saving bundle %s"), *BundleId.ToString());
    
    // Use the asset manager's SaveBundle method to save this bundle to the project
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    return AssetManager.SaveBundle(this, TEXT("/Game/Bundles"));
} 