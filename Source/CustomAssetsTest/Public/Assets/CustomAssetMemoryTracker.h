#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CustomAssetMemoryTracker.generated.h"

/**
 * Struct to represent memory usage statistics for an asset
 */
USTRUCT(BlueprintType)
struct CUSTOMASSETSTEST_API FAssetMemoryStats
{
    GENERATED_BODY()

    // Asset ID
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Memory")
    FName AssetId;

    // Memory usage in bytes
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Memory")
    int64 MemoryUsage;

    // Peak memory usage in bytes
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Memory")
    int64 PeakMemoryUsage;

    // Last time the asset was accessed
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Memory")
    FDateTime LastAccessTime;

    // Number of times the asset has been accessed
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Memory")
    int32 AccessCount;

    // Whether the asset is currently loaded
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Memory")
    bool bIsLoaded;

    // Default constructor
    FAssetMemoryStats()
        : AssetId(NAME_None)
        , MemoryUsage(0)
        , PeakMemoryUsage(0)
        , LastAccessTime(FDateTime::Now())
        , AccessCount(0)
        , bIsLoaded(false)
    {
    }
};

/**
 * Class for tracking memory usage of assets
 */
UCLASS(BlueprintType)
class CUSTOMASSETSTEST_API UCustomAssetMemoryTracker : public UObject
{
    GENERATED_BODY()

public:
    UCustomAssetMemoryTracker();

    // Get the singleton instance
    static UCustomAssetMemoryTracker& Get();

    // Track memory usage for an asset
    UFUNCTION(BlueprintCallable, Category = "Memory")
    void TrackAsset(const FName& AssetId, int64 MemoryUsage);

    // Update memory usage for an asset
    UFUNCTION(BlueprintCallable, Category = "Memory")
    void UpdateAssetMemoryUsage(const FName& AssetId, int64 MemoryUsage);

    // Record an access to an asset
    UFUNCTION(BlueprintCallable, Category = "Memory")
    void RecordAssetAccess(const FName& AssetId);

    // Set the loaded state of an asset
    UFUNCTION(BlueprintCallable, Category = "Memory")
    void SetAssetLoadedState(const FName& AssetId, bool bIsLoaded);

    // Get memory stats for an asset
    UFUNCTION(BlueprintCallable, Category = "Memory")
    FAssetMemoryStats GetAssetMemoryStats(const FName& AssetId) const;

    // Get total memory usage for all tracked assets
    UFUNCTION(BlueprintCallable, Category = "Memory")
    int64 GetTotalMemoryUsage() const;

    // Get total memory usage for loaded assets
    UFUNCTION(BlueprintCallable, Category = "Memory")
    int64 GetLoadedMemoryUsage() const;

    // Get all memory stats
    UFUNCTION(BlueprintCallable, Category = "Memory")
    TArray<FAssetMemoryStats> GetAllMemoryStats() const;

    // Get least recently used assets
    UFUNCTION(BlueprintCallable, Category = "Memory")
    TArray<FName> GetLeastRecentlyUsedAssets(int32 Count) const;

    // Get most frequently used assets
    UFUNCTION(BlueprintCallable, Category = "Memory")
    TArray<FName> GetMostFrequentlyUsedAssets(int32 Count) const;

    // Export memory stats to CSV
    UFUNCTION(BlueprintCallable, Category = "Memory")
    bool ExportMemoryStatsToCSV(const FString& FilePath) const;

private:
    // Map of asset IDs to memory stats
    TMap<FName, FAssetMemoryStats> MemoryStats;

    // Singleton instance
    static UCustomAssetMemoryTracker* Instance;
}; 