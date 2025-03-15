#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Assets/CustomAssetBase.h"
#include "CustomAssetBundle.generated.h"

/**
 * Asset bundle for grouping related assets together
 */
UCLASS(BlueprintType)
class CUSTOMASSETSTEST_API UCustomAssetBundle : public UObject
{
    GENERATED_BODY()

public:
    UCustomAssetBundle();

    // Bundle ID
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bundle")
    FName BundleId;

    // Display name
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bundle")
    FText DisplayName;

    // Bundle name - deprecated, use DisplayName instead
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bundle")
    FText BundleName;

    // Bundle description
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bundle", meta = (MultiLine = true))
    FText Description;

    // Asset IDs in this bundle
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bundle")
    TArray<FName> AssetIds;

    // Tags for categorizing and filtering bundles
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bundle")
    TArray<FName> Tags;

    // Priority for loading (higher priority bundles are loaded first)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bundle", meta = (ClampMin = "0", ClampMax = "100"))
    int32 Priority;

    // Whether to preload this bundle at startup
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bundle")
    bool bPreloadAtStartup;

    // Whether to keep this bundle in memory once loaded
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bundle")
    bool bKeepInMemory;
    
    // Whether all assets in this bundle are currently loaded
    UPROPERTY(BlueprintReadOnly, Category = "Bundle")
    bool bIsLoaded;
    
    // Assets in this bundle (references to the actual asset objects)
    UPROPERTY(Transient)
    TArray<UCustomAssetBase*> Assets;

    // Add an asset to the bundle
    UFUNCTION(BlueprintCallable, Category = "Bundle")
    void AddAsset(const FName& AssetId);

    // Remove an asset from the bundle
    UFUNCTION(BlueprintCallable, Category = "Bundle")
    void RemoveAsset(const FName& AssetId);

    // Check if the bundle contains an asset
    UFUNCTION(BlueprintCallable, Category = "Bundle")
    bool ContainsAsset(const FName& AssetId) const;
    
    // Save the bundle 
    UFUNCTION(BlueprintCallable, Category = "Bundle")
    bool Save();
    
    // Debug function to print the bundle's contents
    UFUNCTION(BlueprintCallable, Category = "Bundle")
    void DebugPrintContents(const FString& Context = TEXT("")) const;
}; 