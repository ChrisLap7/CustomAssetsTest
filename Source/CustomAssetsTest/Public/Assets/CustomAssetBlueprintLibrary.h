#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Assets/CustomAssetBase.h"
#include "Assets/CustomAssetBundle.h"
#include "Assets/CustomAssetManager.h"
#include "Assets/CustomItemAsset.h"
#include "Assets/CustomCharacterAsset.h"
#include "CustomAssetBlueprintLibrary.generated.h"

/**
 * Blueprint function library for working with the Custom Asset System.
 * Provides Blueprint-accessible functions for loading, unloading, and querying assets.
 */
UCLASS()
class CUSTOMASSETSTEST_API UCustomAssetBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Gets a reference to the Custom Asset Manager.
     * @return The Custom Asset Manager instance.
     */
    UFUNCTION(BlueprintPure, Category = "Custom Asset System")
    static UCustomAssetManager* GetCustomAssetManager();
    
    /**
     * Loads an asset by its unique ID.
     * @param AssetId The unique identifier of the asset to load.
     * @return The loaded asset, or nullptr if it could not be loaded.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Assets")
    static UCustomAssetBase* LoadAssetById(FName AssetId);
    
    /**
     * Loads an asset by its unique ID using a specific loading strategy.
     * @param AssetId The unique identifier of the asset to load.
     * @param Strategy The loading strategy to use.
     * @return The loaded asset, or nullptr if it could not be loaded.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Assets")
    static UCustomAssetBase* LoadAssetByIdWithStrategy(FName AssetId, EAssetLoadingStrategy Strategy);
    
    /**
     * Unloads an asset by its unique ID.
     * @param AssetId The unique identifier of the asset to unload.
     * @return True if the asset was successfully unloaded.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Assets")
    static bool UnloadAssetById(FName AssetId);
    
    /**
     * Gets a reference to an already loaded asset by its unique ID.
     * @param AssetId The unique identifier of the asset to get.
     * @return The asset, or nullptr if it is not loaded.
     */
    UFUNCTION(BlueprintPure, Category = "Custom Asset System|Assets")
    static UCustomAssetBase* GetAssetById(FName AssetId);
    
    /**
     * Gets all loaded asset IDs.
     * @return An array of all loaded asset IDs.
     */
    UFUNCTION(BlueprintPure, Category = "Custom Asset System|Assets")
    static TArray<FName> GetAllLoadedAssetIds();
    
    /**
     * Gets all registered asset IDs (not necessarily loaded).
     * @return An array of all registered asset IDs.
     */
    UFUNCTION(BlueprintPure, Category = "Custom Asset System|Assets")
    static TArray<FName> GetAllAssetIds();
    
    /**
     * Preloads a set of assets.
     * @param AssetIds The unique identifiers of the assets to preload.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Assets")
    static void PreloadAssets(const TArray<FName>& AssetIds);
    
    /**
     * Loads an asset bundle by its unique ID.
     * @param BundleId The unique identifier of the bundle to load.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Bundles")
    static void LoadBundle(FName BundleId);
    
    /**
     * Loads an asset bundle by its unique ID using a specific loading strategy.
     * @param BundleId The unique identifier of the bundle to load.
     * @param Strategy The loading strategy to use.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Bundles")
    static void LoadBundleWithStrategy(FName BundleId, EAssetLoadingStrategy Strategy);
    
    /**
     * Unloads an asset bundle by its unique ID.
     * @param BundleId The unique identifier of the bundle to unload.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Bundles")
    static void UnloadBundle(FName BundleId);
    
    /**
     * Gets a reference to an asset bundle by its unique ID.
     * @param BundleId The unique identifier of the bundle to get.
     * @return The bundle, or nullptr if it is not found.
     */
    UFUNCTION(BlueprintPure, Category = "Custom Asset System|Bundles")
    static UCustomAssetBundle* GetBundleById(FName BundleId);
    
    /**
     * Gets all bundle IDs.
     * @return An array of all bundle IDs.
     */
    UFUNCTION(BlueprintPure, Category = "Custom Asset System|Bundles")
    static TArray<FName> GetAllBundleIds();
    
    /**
     * Gets the assets in a bundle.
     * @param BundleId The unique identifier of the bundle.
     * @return An array of asset IDs in the bundle.
     */
    UFUNCTION(BlueprintPure, Category = "Custom Asset System|Bundles")
    static TArray<FName> GetAssetsInBundle(FName BundleId);
    
    /**
     * Gets the dependencies of an asset.
     * @param AssetId The unique identifier of the asset.
     * @param bHardDependenciesOnly Whether to get only hard dependencies.
     * @return An array of asset IDs that this asset depends on.
     */
    UFUNCTION(BlueprintPure, Category = "Custom Asset System|Dependencies")
    static TArray<FName> GetAssetDependencies(FName AssetId, bool bHardDependenciesOnly);
    
    /**
     * Gets the assets that depend on this asset.
     * @param AssetId The unique identifier of the asset.
     * @param bHardDependenciesOnly Whether to get only hard dependencies.
     * @return An array of asset IDs that depend on this asset.
     */
    UFUNCTION(BlueprintPure, Category = "Custom Asset System|Dependencies")
    static TArray<FName> GetDependentAssets(FName AssetId, bool bHardDependenciesOnly);
    
    /**
     * Exports the dependency graph to a DOT file.
     * @param FilePath The path to save the DOT file to.
     * @return True if the file was successfully exported.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Dependencies")
    static bool ExportDependencyGraph(const FString& FilePath);
    
    /**
     * Gets the current memory usage of loaded assets in megabytes.
     * @return The memory usage in megabytes.
     */
    UFUNCTION(BlueprintPure, Category = "Custom Asset System|Memory")
    static int32 GetCurrentMemoryUsage();
    
    /**
     * Gets the memory usage threshold in megabytes.
     * @return The memory threshold in megabytes.
     */
    UFUNCTION(BlueprintPure, Category = "Custom Asset System|Memory")
    static int32 GetMemoryUsageThreshold();
    
    /**
     * Sets the memory usage threshold in megabytes.
     * @param ThresholdMB The memory threshold in megabytes.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Memory")
    static void SetMemoryUsageThreshold(int32 ThresholdMB);
    
    /**
     * Sets the memory management policy.
     * @param Policy The memory management policy to use.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Memory")
    static void SetMemoryManagementPolicy(EMemoryManagementPolicy Policy);
    
    /**
     * Exports memory usage statistics to a CSV file.
     * @param FilePath The path to save the CSV file to.
     * @return True if the file was successfully exported.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Memory")
    static bool ExportMemoryUsageToCSV(const FString& FilePath);
    
    /**
     * Gets the memory stats for a specific asset.
     * @param AssetId The unique identifier of the asset.
     * @return The memory stats for the asset.
     */
    UFUNCTION(BlueprintPure, Category = "Custom Asset System|Memory")
    static FAssetMemoryStats GetAssetMemoryStats(FName AssetId);
    
    /**
     * Exports asset information to a CSV file.
     * @param FilePath The path to save the CSV file to.
     * @return True if the file was successfully exported.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Utilities")
    static bool ExportAssetsToCSV(const FString& FilePath);
    
    /**
     * Gets the display name of an asset.
     * @param AssetId The unique identifier of the asset.
     * @return The display name of the asset.
     */
    UFUNCTION(BlueprintPure, Category = "Custom Asset System|Utilities")
    static FText GetAssetDisplayName(FName AssetId);
    
    /**
     * Gets the description of an asset.
     * @param AssetId The unique identifier of the asset.
     * @return The description of the asset.
     */
    UFUNCTION(BlueprintPure, Category = "Custom Asset System|Utilities")
    static FText GetAssetDescription(FName AssetId);
    
    /**
     * Gets the tags of an asset.
     * @param AssetId The unique identifier of the asset.
     * @return The tags of the asset.
     */
    UFUNCTION(BlueprintPure, Category = "Custom Asset System|Utilities")
    static TArray<FName> GetAssetTags(FName AssetId);
    
    /**
     * Gets the version of an asset.
     * @param AssetId The unique identifier of the asset.
     * @return The version of the asset.
     */
    UFUNCTION(BlueprintPure, Category = "Custom Asset System|Utilities")
    static int32 GetAssetVersion(FName AssetId);
    
    /**
     * Gets the version history of an asset.
     * @param AssetId The unique identifier of the asset.
     * @return The version history of the asset.
     */
    UFUNCTION(BlueprintPure, Category = "Custom Asset System|Utilities")
    static TArray<FAssetVersionChange> GetAssetVersionHistory(FName AssetId);

    // Item Asset Functions

    /**
     * Gets an item asset by its unique ID.
     * @param AssetId The unique identifier of the item asset.
     * @param bLoadIfNecessary Whether to load the asset if it's not already loaded.
     * @return The item asset, or nullptr if it doesn't exist or couldn't be loaded.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Item Assets")
    static UCustomItemAsset* GetItemAsset(FName AssetId, bool bLoadIfNecessary = false);

    /**
     * Gets all registered item asset IDs.
     * @return An array of all item asset IDs.
     */
    UFUNCTION(BlueprintPure, Category = "Custom Asset System|Item Assets")
    static TArray<FName> GetAllItemAssetIds();

    /**
     * Gets item assets by quality.
     * @param Quality The quality level to filter by.
     * @param bLoadAssets Whether to load the assets if they're not already loaded.
     * @return An array of item assets matching the specified quality.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Item Assets")
    static TArray<UCustomItemAsset*> GetItemAssetsByQuality(EItemQuality Quality, bool bLoadAssets = false);

    /**
     * Gets item assets by category.
     * @param Category The category to filter by.
     * @param bLoadAssets Whether to load the assets if they're not already loaded.
     * @return An array of item assets matching the specified category.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Item Assets")
    static TArray<UCustomItemAsset*> GetItemAssetsByCategory(FName Category, bool bLoadAssets = false);

    /**
     * Applies item effects to a stats container.
     * @param ItemAsset The item asset to apply effects from.
     * @param Stats The stats container to modify.
     * @return True if the effects were successfully applied.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Item Assets")
    static bool ApplyItemEffects(UCustomItemAsset* ItemAsset, UPARAM(ref) TMap<FName, float>& Stats);

    /**
     * Checks if an entity can use an item based on its requirements.
     * @param ItemAsset The item asset to check requirements for.
     * @param EntityStats The entity's current stats.
     * @return True if the entity meets all requirements to use the item.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Item Assets")
    static bool CanEntityUseItem(UCustomItemAsset* ItemAsset, const TMap<FName, float>& EntityStats);

    // Character Asset Functions

    /**
     * Gets a character asset by its unique ID.
     * @param AssetId The unique identifier of the character asset.
     * @param bLoadIfNecessary Whether to load the asset if it's not already loaded.
     * @return The character asset, or nullptr if it doesn't exist or couldn't be loaded.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Character Assets")
    static UCustomCharacterAsset* GetCharacterAsset(FName AssetId, bool bLoadIfNecessary = false);

    /**
     * Gets all registered character asset IDs.
     * @return An array of all character asset IDs.
     */
    UFUNCTION(BlueprintPure, Category = "Custom Asset System|Character Assets")
    static TArray<FName> GetAllCharacterAssetIds();

    /**
     * Gets character assets by class.
     * @param CharacterClass The character class to filter by.
     * @param bLoadAssets Whether to load the assets if they're not already loaded.
     * @return An array of character assets matching the specified class.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Character Assets")
    static TArray<UCustomCharacterAsset*> GetCharacterAssetsByClass(ECharacterClass CharacterClass, bool bLoadAssets = false);

    /**
     * Gets character assets by level range.
     * @param MinLevel The minimum level (inclusive).
     * @param MaxLevel The maximum level (inclusive).
     * @param bLoadAssets Whether to load the assets if they're not already loaded.
     * @return An array of character assets within the specified level range.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Character Assets")
    static TArray<UCustomCharacterAsset*> GetCharacterAssetsByLevelRange(int32 MinLevel, int32 MaxLevel, bool bLoadAssets = false);

    /**
     * Gets the experience required for a character to reach a specific level.
     * @param CharacterAsset The character asset to check.
     * @param TargetLevel The target level to calculate experience for.
     * @return The amount of experience required to reach the target level.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Character Assets")
    static int32 GetCharacterExperienceForLevel(UCustomCharacterAsset* CharacterAsset, int32 TargetLevel);

    /**
     * Gets all abilities for a character.
     * @param CharacterAsset The character asset to get abilities from.
     * @return An array of ability IDs for the character.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Character Assets")
    static TArray<FName> GetCharacterAbilities(UCustomCharacterAsset* CharacterAsset);

    /**
     * Gets a specific ability from a character.
     * @param CharacterAsset The character asset to get the ability from.
     * @param AbilityId The ID of the ability to get.
     * @param bFound Output parameter indicating whether the ability was found.
     * @return The character ability structure.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Character Assets")
    static FCharacterAbility GetCharacterAbility(UCustomCharacterAsset* CharacterAsset, FName AbilityId, bool& bFound);

    /**
     * Gets abilities unlocked at a specific level.
     * @param CharacterAsset The character asset to check.
     * @param Level The level to check for unlocked abilities.
     * @return An array of ability IDs unlocked at the specified level.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Character Assets")
    static TArray<FName> GetAbilitiesUnlockedAtLevel(UCustomCharacterAsset* CharacterAsset, int32 Level);

    // LOD Functions

    /**
     * Gets the appropriate LOD mesh for an item based on distance.
     * @param ItemAsset The item asset to get the LOD mesh for.
     * @param Distance The distance from the camera or viewer.
     * @return The appropriate mesh for the specified distance.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|LOD")
    static UStaticMesh* GetItemLODMesh(UCustomItemAsset* ItemAsset, float Distance);

    /**
     * Gets the appropriate LOD mesh for a character based on distance.
     * @param CharacterAsset The character asset to get the LOD mesh for.
     * @param Distance The distance from the camera or viewer.
     * @return The appropriate mesh for the specified distance.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|LOD")
    static USkeletalMesh* GetCharacterLODMesh(UCustomCharacterAsset* CharacterAsset, float Distance);

    // Asset Tags Functions
    
    /**
     * Gets all assets with a specific tag.
     * @param Tag The tag to search for.
     * @param bLoadAssets Whether to load the assets if they're not already loaded.
     * @return An array of assets that have the specified tag.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Tags")
    static TArray<UCustomAssetBase*> GetAssetsByTag(FName Tag, bool bLoadAssets = false);
    
    /**
     * Checks if an asset has a specific tag.
     * @param AssetId The unique identifier of the asset to check.
     * @param Tag The tag to check for.
     * @return True if the asset has the specified tag.
     */
    UFUNCTION(BlueprintPure, Category = "Custom Asset System|Tags")
    static bool DoesAssetHaveTag(FName AssetId, FName Tag);
    
    /**
     * Adds a tag to an asset.
     * @param AssetId The unique identifier of the asset to modify.
     * @param Tag The tag to add.
     * @return True if the tag was successfully added.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Tags")
    static bool AddTagToAsset(FName AssetId, FName Tag);
    
    /**
     * Removes a tag from an asset.
     * @param AssetId The unique identifier of the asset to modify.
     * @param Tag The tag to remove.
     * @return True if the tag was successfully removed.
     */
    UFUNCTION(BlueprintCallable, Category = "Custom Asset System|Tags")
    static bool RemoveTagFromAsset(FName AssetId, FName Tag);
}; 