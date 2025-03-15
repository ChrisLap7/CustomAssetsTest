#include "Assets/CustomAssetMemoryTracker.h"
#include "Misc/FileHelper.h"

// Initialize the singleton instance
UCustomAssetMemoryTracker* UCustomAssetMemoryTracker::Instance = nullptr;

UCustomAssetMemoryTracker::UCustomAssetMemoryTracker()
{
    // Set the singleton instance
    if (!Instance)
    {
        Instance = this;
    }
}

UCustomAssetMemoryTracker& UCustomAssetMemoryTracker::Get()
{
    if (!Instance)
    {
        // Create a new instance if one doesn't exist
        Instance = NewObject<UCustomAssetMemoryTracker>();
        Instance->AddToRoot(); // Prevent garbage collection
    }
    
    return *Instance;
}

void UCustomAssetMemoryTracker::TrackAsset(const FName& AssetId, int64 MemoryUsage)
{
    // Create new memory stats for the asset
    FAssetMemoryStats Stats;
    Stats.AssetId = AssetId;
    Stats.MemoryUsage = MemoryUsage;
    Stats.PeakMemoryUsage = MemoryUsage;
    Stats.LastAccessTime = FDateTime::Now();
    Stats.AccessCount = 1;
    Stats.bIsLoaded = true;

    // Add to the map
    MemoryStats.Add(AssetId, Stats);
}

void UCustomAssetMemoryTracker::UpdateAssetMemoryUsage(const FName& AssetId, int64 MemoryUsage)
{
    // Check if the asset is being tracked
    FAssetMemoryStats* Stats = MemoryStats.Find(AssetId);
    if (!Stats)
    {
        // Start tracking the asset
        TrackAsset(AssetId, MemoryUsage);
        return;
    }

    // Update memory usage
    Stats->MemoryUsage = MemoryUsage;

    // Update peak memory usage if needed
    if (MemoryUsage > Stats->PeakMemoryUsage)
    {
        Stats->PeakMemoryUsage = MemoryUsage;
    }
}

void UCustomAssetMemoryTracker::RecordAssetAccess(const FName& AssetId)
{
    // Check if the asset is being tracked
    FAssetMemoryStats* Stats = MemoryStats.Find(AssetId);
    if (!Stats)
    {
        return;
    }

    // Update access time and count
    Stats->LastAccessTime = FDateTime::Now();
    Stats->AccessCount++;
}

void UCustomAssetMemoryTracker::SetAssetLoadedState(const FName& AssetId, bool bIsLoaded)
{
    // Check if the asset is being tracked
    FAssetMemoryStats* Stats = MemoryStats.Find(AssetId);
    if (!Stats)
    {
        return;
    }

    // Update loaded state
    Stats->bIsLoaded = bIsLoaded;

    // If unloaded, set memory usage to 0
    if (!bIsLoaded)
    {
        Stats->MemoryUsage = 0;
    }
}

FAssetMemoryStats UCustomAssetMemoryTracker::GetAssetMemoryStats(const FName& AssetId) const
{
    // Check if the asset is being tracked
    const FAssetMemoryStats* Stats = MemoryStats.Find(AssetId);
    if (!Stats)
    {
        // Return default stats
        FAssetMemoryStats DefaultStats;
        DefaultStats.AssetId = AssetId;
        return DefaultStats;
    }

    return *Stats;
}

int64 UCustomAssetMemoryTracker::GetTotalMemoryUsage() const
{
    int64 TotalMemory = 0;

    // Sum up memory usage for all assets
    for (const auto& Pair : MemoryStats)
    {
        TotalMemory += Pair.Value.MemoryUsage;
    }

    return TotalMemory;
}

int64 UCustomAssetMemoryTracker::GetLoadedMemoryUsage() const
{
    int64 LoadedMemory = 0;

    // Sum up memory usage for loaded assets
    for (const auto& Pair : MemoryStats)
    {
        if (Pair.Value.bIsLoaded)
        {
            LoadedMemory += Pair.Value.MemoryUsage;
        }
    }

    return LoadedMemory;
}

TArray<FAssetMemoryStats> UCustomAssetMemoryTracker::GetAllMemoryStats() const
{
    TArray<FAssetMemoryStats> AllStats;

    // Collect all memory stats
    for (const auto& Pair : MemoryStats)
    {
        AllStats.Add(Pair.Value);
    }

    return AllStats;
}

TArray<FName> UCustomAssetMemoryTracker::GetLeastRecentlyUsedAssets(int32 Count) const
{
    TArray<FName> LRUAssets;
    TArray<FAssetMemoryStats> AllStats = GetAllMemoryStats();

    // Sort by last access time (oldest first)
    AllStats.Sort([](const FAssetMemoryStats& A, const FAssetMemoryStats& B) {
        return A.LastAccessTime < B.LastAccessTime;
    });

    // Get the specified number of assets
    int32 NumToGet = FMath::Min(Count, AllStats.Num());
    for (int32 i = 0; i < NumToGet; ++i)
    {
        LRUAssets.Add(AllStats[i].AssetId);
    }

    return LRUAssets;
}

TArray<FName> UCustomAssetMemoryTracker::GetMostFrequentlyUsedAssets(int32 Count) const
{
    TArray<FName> MFUAssets;
    TArray<FAssetMemoryStats> AllStats = GetAllMemoryStats();

    // Sort by access count (highest first)
    AllStats.Sort([](const FAssetMemoryStats& A, const FAssetMemoryStats& B) {
        return A.AccessCount > B.AccessCount;
    });

    // Get the specified number of assets
    int32 NumToGet = FMath::Min(Count, AllStats.Num());
    for (int32 i = 0; i < NumToGet; ++i)
    {
        MFUAssets.Add(AllStats[i].AssetId);
    }

    return MFUAssets;
}

bool UCustomAssetMemoryTracker::ExportMemoryStatsToCSV(const FString& FilePath) const
{
    FString CSVContent = "AssetId,MemoryUsage,PeakMemoryUsage,LastAccessTime,AccessCount,IsLoaded\n";

    // Get all memory stats
    TArray<FAssetMemoryStats> AllStats = GetAllMemoryStats();

    // Sort by memory usage (highest first)
    AllStats.Sort([](const FAssetMemoryStats& A, const FAssetMemoryStats& B) {
        return A.MemoryUsage > B.MemoryUsage;
    });

    // Add each asset's stats to the CSV
    for (const FAssetMemoryStats& Stats : AllStats)
    {
        CSVContent += FString::Printf(TEXT("%s,%lld,%lld,%s,%d,%s\n"),
            *Stats.AssetId.ToString(),
            Stats.MemoryUsage,
            Stats.PeakMemoryUsage,
            *Stats.LastAccessTime.ToString(),
            Stats.AccessCount,
            Stats.bIsLoaded ? TEXT("True") : TEXT("False"));
    }

    // Write the CSV file
    return FFileHelper::SaveStringToFile(CSVContent, *FilePath);
} 