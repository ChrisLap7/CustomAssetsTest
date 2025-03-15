// Copyright Epic Games, Inc. All Rights Reserved.

#include "Assets/CustomAssetBase.h"
#include "UObject/ObjectSaveContext.h"

UCustomAssetBase::UCustomAssetBase()
{
    // Initialize default values
    Version = 1;
    MinCompatibleVersion = 1;
    LastModified = FDateTime::Now();

    // Add initial version to history
    FAssetVersionChange InitialVersion;
    InitialVersion.VersionNumber = 1;
    InitialVersion.ChangeType = ECustomAssetVersionType::Initial;
    InitialVersion.ChangeDescription = FText::FromString("Initial version");
    InitialVersion.ChangeTimestamp = FDateTime::Now();
    VersionHistory.Add(InitialVersion);
}

FPrimaryAssetId UCustomAssetBase::GetPrimaryAssetId() const
{
    // Use the asset's unique ID as the primary asset ID
    // If AssetId is not set, fall back to the parent implementation
    if (AssetId.IsNone())
    {
        return Super::GetPrimaryAssetId();
    }
    
    // Use the class name as the primary asset type and the AssetId as the primary asset name
    return FPrimaryAssetId(GetClass()->GetFName(), AssetId);
}

void UCustomAssetBase::AddDependency(const FName& DependentAssetId, const FName& DependencyType, bool bHardDependency)
{
    // Check if the dependency already exists
    for (int32 i = 0; i < Dependencies.Num(); ++i)
    {
        if (Dependencies[i].DependentAssetId == DependentAssetId)
        {
            // Update the existing dependency
            Dependencies[i].DependencyType = DependencyType;
            Dependencies[i].bHardDependency = bHardDependency;
            return;
        }
    }

    // Add a new dependency
    Dependencies.Add(FCustomAssetDependency(DependentAssetId, DependencyType, bHardDependency));
}

void UCustomAssetBase::RemoveDependency(const FName& DependentAssetId)
{
    // Find and remove the dependency
    for (int32 i = 0; i < Dependencies.Num(); ++i)
    {
        if (Dependencies[i].DependentAssetId == DependentAssetId)
        {
            Dependencies.RemoveAt(i);
            return;
        }
    }
}

bool UCustomAssetBase::DependsOn(const FName& DependentAssetId) const
{
    // Check if the asset depends on the specified asset
    for (const FCustomAssetDependency& Dependency : Dependencies)
    {
        if (Dependency.DependentAssetId == DependentAssetId)
        {
            return true;
        }
    }

    return false;
}

TArray<FName> UCustomAssetBase::GetHardDependencies() const
{
    TArray<FName> HardDependencies;

    // Collect all hard dependencies
    for (const FCustomAssetDependency& Dependency : Dependencies)
    {
        if (Dependency.bHardDependency)
        {
            HardDependencies.Add(Dependency.DependentAssetId);
        }
    }

    return HardDependencies;
}

TArray<FName> UCustomAssetBase::GetSoftDependencies() const
{
    TArray<FName> SoftDependencies;

    // Collect all soft dependencies
    for (const FCustomAssetDependency& Dependency : Dependencies)
    {
        if (!Dependency.bHardDependency)
        {
            SoftDependencies.Add(Dependency.DependentAssetId);
        }
    }

    return SoftDependencies;
}

void UCustomAssetBase::AddDependentAsset(const FName& InAssetId, const FName& DependencyType, bool bHardDependency)
{
    // Create a new dependency
    FCustomAssetDependency Dependency;
    Dependency.DependentAssetId = InAssetId;
    Dependency.DependencyType = DependencyType;
    Dependency.bHardDependency = bHardDependency;
    
    // Check if the dependency already exists
    for (int32 i = 0; i < DependentAssets.Num(); ++i)
    {
        if (DependentAssets[i].DependentAssetId == InAssetId)
        {
            // Update the existing dependency
            DependentAssets[i] = Dependency;
            return;
        }
    }
    
    // Add the new dependency
    DependentAssets.Add(Dependency);
}

void UCustomAssetBase::RemoveDependentAsset(const FName& InAssetId)
{
    // Find and remove the dependent asset
    for (int32 i = 0; i < DependentAssets.Num(); ++i)
    {
        if (DependentAssets[i].DependentAssetId == InAssetId)
        {
            DependentAssets.RemoveAt(i);
            return;
        }
    }
}

void UCustomAssetBase::UpdateVersion(ECustomAssetVersionType ChangeType, const FText& ChangeDescription)
{
    // Increment version number
    Version++;

    // Update minimum compatible version for breaking changes
    if (ChangeType == ECustomAssetVersionType::Breaking)
    {
        MinCompatibleVersion = Version;
    }

    // Add version change to history
    FAssetVersionChange VersionChange;
    VersionChange.VersionNumber = Version;
    VersionChange.ChangeType = ChangeType;
    VersionChange.ChangeDescription = ChangeDescription;
    VersionChange.ChangeTimestamp = FDateTime::Now();
    VersionHistory.Add(VersionChange);

    // Update last modified timestamp
    LastModified = FDateTime::Now();
}

bool UCustomAssetBase::IsCompatibleWithVersion(int32 OtherVersion) const
{
    return OtherVersion >= MinCompatibleVersion && OtherVersion <= Version;
}

TArray<FAssetVersionChange> UCustomAssetBase::GetVersionChangesSince(int32 OtherVersion) const
{
    TArray<FAssetVersionChange> Changes;

    for (const FAssetVersionChange& VersionChange : VersionHistory)
    {
        if (VersionChange.VersionNumber > OtherVersion)
        {
            Changes.Add(VersionChange);
        }
    }

    return Changes;
}

bool UCustomAssetBase::MigrateFromVersion(int32 OldVersion)
{
    // Base implementation just returns true
    // Derived classes should override this to perform actual migration
    return true;
}

void UCustomAssetBase::PostLoad()
{
    Super::PostLoad();

    // Check if we need to migrate from an older version
    int32 OldVersion = Version;
    if (OldVersion < Version)
    {
        MigrateFromVersion(OldVersion);
    }
}

void UCustomAssetBase::PreSave(FObjectPreSaveContext SaveContext)
{
    Super::PreSave(SaveContext);

    // Update the last modified timestamp
    LastModified = FDateTime::Now();
} 