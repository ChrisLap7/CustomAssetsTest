// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Assets/CustomAssetVersion.h"
#include "CustomAssetBase.generated.h"

/**
 * Struct to represent an asset dependency
 */
USTRUCT(BlueprintType)
struct CUSTOMASSETSTEST_API FCustomAssetDependency
{
    GENERATED_BODY()

    // The ID of the dependent asset
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dependency")
    FName DependentAssetId;

    // The type of dependency
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dependency")
    FName DependencyType;

    // Whether this is a hard dependency (required) or soft dependency (optional)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dependency")
    bool bHardDependency;

    // Default constructor
    FCustomAssetDependency()
        : DependentAssetId(NAME_None)
        , DependencyType(NAME_None)
        , bHardDependency(true)
    {
    }

    // Constructor with parameters
    FCustomAssetDependency(FName InDependentAssetId, FName InDependencyType, bool bInHardDependency)
        : DependentAssetId(InDependentAssetId)
        , DependencyType(InDependencyType)
        , bHardDependency(bInHardDependency)
    {
    }

    // Equality operator
    bool operator==(const FCustomAssetDependency& Other) const
    {
        return DependentAssetId == Other.DependentAssetId &&
               DependencyType == Other.DependencyType &&
               bHardDependency == Other.bHardDependency;
    }
};

/**
 * Base class for all custom assets in the project
 * Provides common functionality and properties for all asset types
 */
UCLASS(Abstract, BlueprintType)
class CUSTOMASSETSTEST_API UCustomAssetBase : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UCustomAssetBase();

    // Unique identifier for the asset
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset")
    FName AssetId;

    // Display name for the asset
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset")
    FText DisplayName;

    // Description of the asset
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset", meta = (MultiLine = true))
    FText Description;

    // Tags for categorizing and filtering assets
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset")
    TArray<FName> Tags;

    // Asset version for tracking changes
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset")
    int32 Version;

    // Last modified timestamp
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset")
    FDateTime LastModified;

    // Dependencies on other assets
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset Dependencies")
    TArray<FCustomAssetDependency> Dependencies;

    // Assets that depend on this asset (filled at runtime)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Asset Dependencies")
    TArray<FCustomAssetDependency> DependentAssets;

    // Version history
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset Versioning")
    TArray<FAssetVersionChange> VersionHistory;

    // Minimum compatible version
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset Versioning")
    int32 MinCompatibleVersion;

    // Override to provide asset-specific tags for the asset manager
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    // Add a dependency to another asset
    UFUNCTION(BlueprintCallable, Category = "Asset Dependencies")
    void AddDependency(const FName& DependentAssetId, const FName& DependencyType, bool bHardDependency = true);

    // Remove a dependency to another asset
    UFUNCTION(BlueprintCallable, Category = "Asset Dependencies")
    void RemoveDependency(const FName& DependentAssetId);

    // Check if this asset depends on another asset
    UFUNCTION(BlueprintCallable, Category = "Asset Dependencies")
    bool DependsOn(const FName& DependentAssetId) const;

    // Get all hard dependencies
    UFUNCTION(BlueprintCallable, Category = "Asset Dependencies")
    TArray<FName> GetHardDependencies() const;

    // Get all soft dependencies
    UFUNCTION(BlueprintCallable, Category = "Asset Dependencies")
    TArray<FName> GetSoftDependencies() const;

    // Add a dependent asset (called by the asset manager)
    void AddDependentAsset(const FName& AssetId, const FName& DependencyType, bool bHardDependency = true);

    // Remove a dependent asset (called by the asset manager)
    void RemoveDependentAsset(const FName& AssetId);

    // VERSIONING FUNCTIONS

    // Update the asset version
    UFUNCTION(BlueprintCallable, Category = "Asset Versioning")
    void UpdateVersion(ECustomAssetVersionType ChangeType, const FText& ChangeDescription);

    // Check if this asset is compatible with another version
    UFUNCTION(BlueprintCallable, Category = "Asset Versioning")
    bool IsCompatibleWithVersion(int32 OtherVersion) const;

    // Get all version changes since a specific version
    UFUNCTION(BlueprintCallable, Category = "Asset Versioning")
    TArray<FAssetVersionChange> GetVersionChangesSince(int32 OtherVersion) const;

    // Migrate from an older version (override in derived classes)
    UFUNCTION(BlueprintCallable, Category = "Asset Versioning")
    virtual bool MigrateFromVersion(int32 OldVersion);

protected:
    // Called when the asset is loaded
    virtual void PostLoad() override;

    // Called when the asset is saved
    virtual void PreSave(FObjectPreSaveContext SaveContext) override;
}; 