#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "Assets/CustomAssetBase.h"
#include "Containers/Map.h"
#include "Engine/StreamableManager.h"
#include "CustomAssetManager.generated.h"

// Forward declarations
class UCustomAssetBundle;
class UCustomAssetMemoryTracker;

// Define a delegate for asset loading completion
DECLARE_DYNAMIC_DELEGATE(FOnAssetLoaded);

/**
 * Enum defining different asset loading strategies
 */
UENUM(BlueprintType)
enum class EAssetLoadingStrategy : uint8
{
    // Load assets on-demand when requested
    OnDemand UMETA(DisplayName = "On Demand"),
    
    // Preload assets at startup
    Preload UMETA(DisplayName = "Preload"),
    
    // Stream assets in the background
    Streaming UMETA(DisplayName = "Streaming"),
    
    // Lazy load assets when needed but keep them in memory
    LazyLoad UMETA(DisplayName = "Lazy Load")
};

/**
 * Enum defining different memory management policies
 */
UENUM(BlueprintType)
enum class EMemoryManagementPolicy : uint8
{
    // Keep all assets in memory once loaded
    KeepAll UMETA(DisplayName = "Keep All"),
    
    // Unload least recently used assets when memory usage exceeds a threshold
    UnloadLRU UMETA(DisplayName = "Unload LRU"),
    
    // Unload least frequently used assets when memory usage exceeds a threshold
    UnloadLFU UMETA(DisplayName = "Unload LFU"),
    
    // Custom policy defined by the user
    Custom UMETA(DisplayName = "Custom")
};

/**
 * Enum defining different asset compression tiers
 */
UENUM(BlueprintType)
enum class EAssetCompressionTier : uint8
{
    // No compression, fastest loading but largest size
    None UMETA(DisplayName = "None"),
    
    // Low compression, good balance for critical assets
    Low UMETA(DisplayName = "Low"),
    
    // Medium compression, default for most assets
    Medium UMETA(DisplayName = "Medium"),
    
    // High compression, smallest size but slower loading
    High UMETA(DisplayName = "High")
};

/**
 * Information about an asset hotswap event
 */
USTRUCT(BlueprintType)
struct FAssetHotswapInfo
{
    GENERATED_BODY()
    
    // ID of the asset that was hotswapped
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Hotswapping")
    FName AssetId;
    
    FAssetHotswapInfo() : AssetId(NAME_None) {}
};

/**
 * Struct to associate bundles with levels for streaming
 */
USTRUCT(BlueprintType)
struct FBundleLevelAssociation
{
    GENERATED_BODY()
    
    // ID of the bundle to load
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Streaming")
    FName BundleId;
    
    // Name of the level this bundle is associated with
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Streaming")
    FName LevelName;
    
    // Distance at which to preload the bundle (in world units)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Streaming")
    float PreloadDistance = 5000.0f;
    
    // Whether to automatically unload the bundle when the level is unloaded
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Streaming")
    bool bUnloadWithLevel = true;
    
    FBundleLevelAssociation() : BundleId(NAME_None), LevelName(NAME_None), PreloadDistance(5000.0f), bUnloadWithLevel(true) {}
};

/**
 * Custom asset manager for handling loading, unloading, and tracking custom assets
 */
UCLASS()
class CUSTOMASSETSTEST_API UCustomAssetManager : public UAssetManager
{
    GENERATED_BODY()

public:
    UCustomAssetManager();

    // Get the singleton instance of the asset manager
    static UCustomAssetManager& Get();

    // Initialize the asset manager
    virtual void StartInitialLoading() override;

    // Load an asset by its ID
    UFUNCTION(BlueprintCallable, Category = "Asset Management")
    UCustomAssetBase* LoadAssetById(const FName& AssetId);

    // Load an asset by its ID with a specific loading strategy
    UFUNCTION(BlueprintCallable, Category = "Asset Management")
    UCustomAssetBase* LoadAssetByIdWithStrategy(const FName& AssetId, EAssetLoadingStrategy Strategy);

    // Preload a list of assets by their IDs
    UFUNCTION(BlueprintCallable, Category = "Asset Management")
    void PreloadAssets(const TArray<FName>& AssetIds);

    // Stream an asset in the background
    UFUNCTION(BlueprintCallable, Category = "Asset Management")
    void StreamAsset(const FName& AssetId, const FOnAssetLoaded& CompletionCallback);

    // Set the default loading strategy for all assets
    UFUNCTION(BlueprintCallable, Category = "Asset Management")
    void SetDefaultLoadingStrategy(EAssetLoadingStrategy Strategy);

    // Unload an asset by its ID
    UFUNCTION(BlueprintCallable, Category = "Asset Management")
    bool UnloadAssetById(const FName& AssetId);

    // Get an asset by its ID (returns nullptr if not loaded)
    UFUNCTION(BlueprintCallable, Category = "Asset Management")
    UCustomAssetBase* GetAssetById(const FName& AssetId) const;

    // Get all loaded assets
    UFUNCTION(BlueprintCallable, Category = "Asset Management")
    TArray<UCustomAssetBase*> GetAllLoadedAssets() const;

    // Get all asset IDs
    UFUNCTION(BlueprintCallable, Category = "Asset Management")
    TArray<FName> GetAllAssetIds() const;

    // Export asset data to CSV
    UFUNCTION(BlueprintCallable, Category = "Asset Management")
    bool ExportAssetsToCSV(const FString& FilePath) const;

    // Scan for all available assets in the content directory
    UFUNCTION(BlueprintCallable, Category = "Asset Management")
    void ScanForAssets();

    // Map of asset IDs to asset paths - made public for use by CustomAssetTable
    TMap<FName, FSoftObjectPath> AssetPathMap;

    // ASSET BUNDLE FUNCTIONS

    // Create a new bundle with the given name
    UFUNCTION(BlueprintCallable, Category = "Asset Bundles")
    UCustomAssetBundle* CreateBundle(const FString& BundleName);

    // Add a bundle to the manager (used by editor tools to create bundles)
    UFUNCTION(BlueprintCallable, Category = "Asset Bundles")
    void AddBundle(UCustomAssetBundle* Bundle);

    // Register an asset bundle
    UFUNCTION(BlueprintCallable, Category = "Asset Bundles")
    void RegisterBundle(UCustomAssetBundle* Bundle);

    // Unregister an asset bundle
    UFUNCTION(BlueprintCallable, Category = "Asset Bundles")
    void UnregisterBundle(UCustomAssetBundle* Bundle);

    // Get a bundle by its ID
    UFUNCTION(BlueprintCallable, Category = "Asset Bundles")
    UCustomAssetBundle* GetBundleById(const FName& BundleId) const;

    // Get all registered bundles
    UFUNCTION(BlueprintCallable, Category = "Asset Bundles")
    void GetAllBundles(TArray<UCustomAssetBundle*>& OutBundles);

    // Load all assets in a bundle
    UFUNCTION(BlueprintCallable, Category = "Asset Bundles")
    void LoadBundle(const FName& BundleId, EAssetLoadingStrategy Strategy = EAssetLoadingStrategy::OnDemand);

    // Unload all assets in a bundle
    UFUNCTION(BlueprintCallable, Category = "Asset Bundles")
    void UnloadBundle(const FName& BundleId);

    // Save a bundle to the project
    UFUNCTION(BlueprintCallable, Category = "Asset Bundles")
    bool SaveBundle(UCustomAssetBundle* Bundle, const FString& PackagePath = TEXT(""));

    // Save all bundles to the project
    UFUNCTION(BlueprintCallable, Category = "Asset Bundles")
    int32 SaveAllBundles(const FString& BasePath = TEXT(""));

    // Scan for all available bundles in the content directory
    UFUNCTION(BlueprintCallable, Category = "Asset Bundles")
    void ScanForBundles();

    // Preload all bundles marked for preloading
    UFUNCTION(BlueprintCallable, Category = "Asset Bundles")
    void PreloadBundles();

    // Get all bundles containing the specified asset
    UFUNCTION(BlueprintCallable, Category = "Asset Bundles")
    TArray<UCustomAssetBundle*> GetAllBundlesContainingAsset(const FName& AssetId) const;

    // Delete a bundle by its ID
    UFUNCTION(BlueprintCallable, Category = "Asset Bundles")
    bool DeleteBundle(const FName& BundleId);

    // Rename a bundle
    UFUNCTION(BlueprintCallable, Category = "Asset Bundles")
    bool RenameBundle(const FName& BundleId, const FString& NewName);

    // DEPENDENCY FUNCTIONS

    // Load all dependencies for an asset
    UFUNCTION(BlueprintCallable, Category = "Asset Dependencies")
    void LoadDependencies(const FName& AssetId, bool bLoadHardDependenciesOnly = false, EAssetLoadingStrategy Strategy = EAssetLoadingStrategy::OnDemand);

    // Get all assets that depend on the specified asset
    UFUNCTION(BlueprintCallable, Category = "Asset Dependencies")
    TArray<FName> GetDependentAssets(const FName& AssetId, bool bHardDependenciesOnly = false) const;

    // Check if an asset can be safely unloaded (no other loaded assets depend on it)
    UFUNCTION(BlueprintCallable, Category = "Asset Dependencies")
    bool CanUnloadAsset(const FName& AssetId) const;

    // Update dependencies between assets
    UFUNCTION(BlueprintCallable, Category = "Asset Dependencies")
    void UpdateDependencies();

    // Export dependency graph to DOT format for visualization
    UFUNCTION(BlueprintCallable, Category = "Asset Dependencies")
    bool ExportDependencyGraph(const FString& FilePath) const;

    // MEMORY MANAGEMENT FUNCTIONS

    // Set the memory management policy
    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    void SetMemoryManagementPolicy(EMemoryManagementPolicy Policy);

    // Set the memory usage threshold in megabytes
    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    void SetMemoryUsageThreshold(int32 ThresholdMB);

    // Get the current memory usage in megabytes
    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    int32 GetCurrentMemoryUsage() const;

    // Get the memory usage threshold in megabytes
    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    int32 GetMemoryUsageThreshold() const;

    // Manage memory usage (unload assets if needed)
    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    void ManageMemoryUsage();

    // Unload assets to free up memory
    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    void UnloadAssetsToFreeMemory(int32 MemoryToFreeMB);

    // Export memory usage stats to CSV
    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    bool ExportMemoryUsageToCSV(const FString& FilePath) const;

    // Get the memory tracker
    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    UCustomAssetMemoryTracker* GetMemoryTracker() const;
    
    // Register an asset with the manager
    UFUNCTION(BlueprintCallable, Category = "Asset Management")
    void RegisterAsset(UCustomAssetBase* Asset);

    // Register an asset path without loading the asset
    UFUNCTION(BlueprintCallable, Category = "Asset Management")
    void RegisterAssetPath(const FName& AssetId, const FSoftObjectPath& AssetPath);

    // Unregister an asset from the manager
    UFUNCTION(BlueprintCallable, Category = "Asset Management")
    void UnregisterAsset(UCustomAssetBase* Asset);

    // Estimate memory usage for an asset
    UFUNCTION(BlueprintCallable, Category = "Memory Management")
    int64 EstimateAssetMemoryUsage(UCustomAssetBase* Asset) const;

    // Helper method for bundle loading completion
    void OnBundleLoaded(FName BundleId);

    // Helper method for async asset loading completion
    void OnAssetLoaded(FName AssetId, FSoftObjectPath AssetPath);

    // ASSET PREFETCHING SYSTEM
    
    // Prefetch assets in a radius around a location
    UFUNCTION(BlueprintCallable, Category = "Asset Prefetching")
    void PrefetchAssetsInRadius(FVector Location, float Radius, int32 MaxAssets = 10);

    // Prefetch specific assets with low priority
    UFUNCTION(BlueprintCallable, Category = "Asset Prefetching")
    void PrefetchAssets(const TArray<FName>& AssetIds);
    
    // Register an asset's world location for spatial prefetching
    UFUNCTION(BlueprintCallable, Category = "Asset Prefetching")
    void RegisterAssetLocation(const FName& AssetId, const FVector& WorldLocation);
    
    // Get assets within a radius of a location
    UFUNCTION(BlueprintCallable, Category = "Asset Prefetching")
    TArray<FName> GetAssetsInRadius(const FVector& Location, float Radius) const;
    
    // Estimate distance between an asset and location (if asset has registered location)
    UFUNCTION(BlueprintCallable, Category = "Asset Prefetching")
    float GetDistanceToAsset(const FName& AssetId, const FVector& FromLocation) const;
    
    // ASSET COMPRESSION TIERS
    
    // Set compression tier for a specific asset
    UFUNCTION(BlueprintCallable, Category = "Asset Compression")
    void SetAssetCompressionTier(const FName& AssetId, EAssetCompressionTier Tier);
    
    // Get compression tier for a specific asset
    UFUNCTION(BlueprintCallable, Category = "Asset Compression")
    EAssetCompressionTier GetAssetCompressionTier(const FName& AssetId) const;

    // Set default compression tier for new assets
    UFUNCTION(BlueprintCallable, Category = "Asset Compression")
    void SetDefaultCompressionTier(EAssetCompressionTier Tier);
    
    // Recompress an asset with a different compression tier
    UFUNCTION(BlueprintCallable, Category = "Asset Compression")
    bool RecompressAsset(const FName& AssetId, EAssetCompressionTier NewTier);
    
    // LEVEL STREAMING INTEGRATION
    
    // Register a bundle to be loaded with a level
    UFUNCTION(BlueprintCallable, Category = "Level Streaming")
    void RegisterBundleWithLevel(FName BundleId, FName LevelName, float PreloadDistance = 5000.f, bool bUnloadWithLevel = true);
    
    // Unregister a bundle from a level
    UFUNCTION(BlueprintCallable, Category = "Level Streaming")
    void UnregisterBundleFromLevel(FName BundleId, FName LevelName);
    
    // Update level-based bundles based on player location
    UFUNCTION(BlueprintCallable, Category = "Level Streaming")
    void UpdateLevelBasedBundles(APlayerController* PlayerController);
    
    // Get distance to a specific level from a location
    UFUNCTION(BlueprintCallable, Category = "Level Streaming")
    float GetDistanceToLevel(FName LevelName, const FVector& FromLocation) const;
    
    // Notify the asset manager that a level has been loaded
    UFUNCTION(BlueprintCallable, Category = "Level Streaming")
    void OnLevelLoaded(FName LevelName);
    
    // Notify the asset manager that a level has been unloaded
    UFUNCTION(BlueprintCallable, Category = "Level Streaming")
    void OnLevelUnloaded(FName LevelName);
    
    // ASSET HOTSWAPPING
    
    // Replace an asset at runtime with a new version
    UFUNCTION(BlueprintCallable, Category = "Asset Hotswapping")
    bool HotswapAsset(const FName& AssetId, UCustomAssetBase* NewAssetVersion);
    
    // Register a listener for asset hotswap events
    UFUNCTION(BlueprintCallable, Category = "Asset Hotswapping")
    void RegisterHotswapListener(UObject* Listener, FName FunctionName);
    
    // Unregister a hotswap listener
    UFUNCTION(BlueprintCallable, Category = "Asset Hotswapping")
    void UnregisterHotswapListener(UObject* Listener);
    
    // Check if an asset has a pending hotswap
    UFUNCTION(BlueprintCallable, Category = "Asset Hotswapping")
    bool HasPendingHotswap(const FName& AssetId) const;
    
    // Apply all pending hotswaps
    UFUNCTION(BlueprintCallable, Category = "Asset Hotswapping")
    int32 ApplyPendingHotswaps();

private:
    // Map of asset IDs to loaded assets
    UPROPERTY()
    TMap<FName, UCustomAssetBase*> LoadedAssets;

    // Map of bundle IDs to bundles
    UPROPERTY()
    TMap<FName, UCustomAssetBundle*> Bundles;

    // Default loading strategy
    EAssetLoadingStrategy DefaultLoadingStrategy;

    // Memory management policy
    EMemoryManagementPolicy MemoryPolicy;

    // Memory usage threshold in bytes
    int64 MemoryThreshold;

    // Memory tracker
    UPROPERTY()
    UCustomAssetMemoryTracker* MemoryTracker;

    // Map to store pending callbacks for streaming assets
    TMap<FName, FOnAssetLoaded> PendingCallbacks;

    // Streamable handle manager for streaming assets
    TSharedPtr<FStreamableHandle> StreamingHandle;

    // Callback for when an asset finishes streaming
    void OnAssetStreamed(FName AssetId);

    // Internal function to register dependencies between assets
    void RegisterAssetDependencies(UCustomAssetBase* Asset);

    // Internal function to unregister dependencies between assets
    void UnregisterAssetDependencies(UCustomAssetBase* Asset);

    // Helper method to calculate memory used by dependencies recursively
    int64 CalculateDependenciesMemoryUsage(UCustomAssetBase* Asset, TSet<FName>& ProcessedAssets) const;

    // Map of asset IDs to their world locations (for prefetching)
    TMap<FName, FVector> AssetLocations;
    
    // Map of asset IDs to their compression tiers
    TMap<FName, EAssetCompressionTier> AssetCompressionTiers;
    
    // Default compression tier for new assets
    EAssetCompressionTier DefaultCompressionTier = EAssetCompressionTier::Medium;
    
    // List of bundle-level associations for level streaming
    UPROPERTY()
    TArray<FBundleLevelAssociation> LevelBundleAssociations;
    
    // Set of currently loaded levels
    TSet<FName> LoadedLevels;
    
    // Map of pending hotswaps (asset ID to new asset version)
    UPROPERTY()
    TMap<FName, UCustomAssetBase*> PendingHotswaps;
    
    // List of hotswap listeners
    TArray<TPair<UObject*, FName>> HotswapListeners;
    
    // Helper function to stream a low-priority asset for prefetching
    void LowPriorityStreamAsset(const FName& AssetId);
    
    // Estimate asset size from metadata without loading
    int64 EstimateAssetSizeFromMetadata(const FName& AssetId) const;
    
    // Notify all hotswap listeners about an asset change
    void NotifyHotswapListeners(const FName& AssetId);
}; 