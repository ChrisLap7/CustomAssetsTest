#include "Assets/CustomAssetVersion.h"

UCustomAssetVersion::UCustomAssetVersion()
{
    // Initialize default values
    CurrentVersion = 1;
    MinCompatibleVersion = 1;

    // Add initial version to history
    FAssetVersionChange InitialVersion;
    InitialVersion.VersionNumber = 1;
    InitialVersion.ChangeType = ECustomAssetVersionType::Initial;
    InitialVersion.ChangeDescription = FText::FromString("Initial version");
    InitialVersion.ChangeTimestamp = FDateTime::Now();
    VersionHistory.Add(InitialVersion);
}

void UCustomAssetVersion::AddVersionChange(ECustomAssetVersionType ChangeType, const FText& ChangeDescription)
{
    // Increment version number
    CurrentVersion++;

    // Update minimum compatible version for breaking changes
    if (ChangeType == ECustomAssetVersionType::Breaking)
    {
        MinCompatibleVersion = CurrentVersion;
    }

    // Add version change to history
    FAssetVersionChange VersionChange;
    VersionChange.VersionNumber = CurrentVersion;
    VersionChange.ChangeType = ChangeType;
    VersionChange.ChangeDescription = ChangeDescription;
    VersionChange.ChangeTimestamp = FDateTime::Now();
    VersionHistory.Add(VersionChange);
}

bool UCustomAssetVersion::IsVersionCompatible(int32 Version) const
{
    return Version >= MinCompatibleVersion && Version <= CurrentVersion;
}

FAssetVersionChange UCustomAssetVersion::GetVersionChange(int32 Version) const
{
    for (const FAssetVersionChange& VersionChange : VersionHistory)
    {
        if (VersionChange.VersionNumber == Version)
        {
            return VersionChange;
        }
    }

    // Return an empty version change if not found
    return FAssetVersionChange();
}

TArray<FAssetVersionChange> UCustomAssetVersion::GetVersionChangesSince(int32 Version) const
{
    TArray<FAssetVersionChange> Changes;

    for (const FAssetVersionChange& VersionChange : VersionHistory)
    {
        if (VersionChange.VersionNumber > Version)
        {
            Changes.Add(VersionChange);
        }
    }

    return Changes;
} 