#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CustomAssetVersion.generated.h"

/**
 * Enum for tracking asset version changes
 */
UENUM(BlueprintType)
enum class ECustomAssetVersionType : uint8
{
    // Initial version
    Initial UMETA(DisplayName = "Initial"),
    
    // Minor update (backward compatible)
    Minor UMETA(DisplayName = "Minor"),
    
    // Major update (may not be backward compatible)
    Major UMETA(DisplayName = "Major"),
    
    // Breaking change (not backward compatible)
    Breaking UMETA(DisplayName = "Breaking")
};

/**
 * Struct to represent a version change
 */
USTRUCT(BlueprintType)
struct CUSTOMASSETSTEST_API FAssetVersionChange
{
    GENERATED_BODY()

    // The version number
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Version")
    int32 VersionNumber;

    // The type of version change
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Version")
    ECustomAssetVersionType ChangeType;

    // Description of the changes
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Version", meta = (MultiLine = true))
    FText ChangeDescription;

    // Timestamp of the change
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Version")
    FDateTime ChangeTimestamp;

    // Default constructor
    FAssetVersionChange()
        : VersionNumber(1)
        , ChangeType(ECustomAssetVersionType::Initial)
        , ChangeTimestamp(FDateTime::Now())
    {
    }

    // Constructor with parameters
    FAssetVersionChange(int32 InVersionNumber, ECustomAssetVersionType InChangeType, const FText& InChangeDescription)
        : VersionNumber(InVersionNumber)
        , ChangeType(InChangeType)
        , ChangeDescription(InChangeDescription)
        , ChangeTimestamp(FDateTime::Now())
    {
    }
};

/**
 * Class for managing asset versioning
 */
UCLASS(BlueprintType)
class CUSTOMASSETSTEST_API UCustomAssetVersion : public UObject
{
    GENERATED_BODY()

public:
    UCustomAssetVersion();

    // The current version number
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Version")
    int32 CurrentVersion;

    // The minimum compatible version
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Version")
    int32 MinCompatibleVersion;

    // History of version changes
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Version")
    TArray<FAssetVersionChange> VersionHistory;

    // Add a new version change
    UFUNCTION(BlueprintCallable, Category = "Version")
    void AddVersionChange(ECustomAssetVersionType ChangeType, const FText& ChangeDescription);

    // Check if a version is compatible with the current version
    UFUNCTION(BlueprintCallable, Category = "Version")
    bool IsVersionCompatible(int32 Version) const;

    // Get the version change for a specific version
    UFUNCTION(BlueprintCallable, Category = "Version")
    FAssetVersionChange GetVersionChange(int32 Version) const;

    // Get all version changes since a specific version
    UFUNCTION(BlueprintCallable, Category = "Version")
    TArray<FAssetVersionChange> GetVersionChangesSince(int32 Version) const;
}; 