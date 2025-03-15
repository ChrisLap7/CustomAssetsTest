#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CustomAssetTable.generated.h"

/**
 * Structure for storing asset references in a data table
 */
USTRUCT(BlueprintType)
struct CUSTOMASSETSTEST_API FCustomAssetTableRow : public FTableRowBase
{
    GENERATED_BODY()

    // Asset ID
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset")
    FName AssetId;

    // Asset path
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset")
    FSoftObjectPath AssetPath;

    // Asset type
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset")
    FName AssetType;

    // Asset display name
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset")
    FText DisplayName;

    // Asset description
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset", meta = (MultiLine = true))
    FText Description;

    // Asset tags
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset")
    TArray<FName> Tags;

    // Asset version
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset")
    int32 Version;

    // Last modified timestamp
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset")
    FString LastModified;

    // Default constructor
    FCustomAssetTableRow()
        : AssetId(NAME_None)
        , AssetType(NAME_None)
        , Version(1)
        , LastModified(FDateTime::Now().ToString())
    {
    }
};

/**
 * Class for managing asset data tables
 */
UCLASS(BlueprintType)
class CUSTOMASSETSTEST_API UCustomAssetTable : public UDataTable
{
    GENERATED_BODY()

public:
    UCustomAssetTable();

    // Export the table to CSV
    UFUNCTION(BlueprintCallable, Category = "Asset Table")
    bool ExportToCSV(const FString& FilePath) const;

    // Import from CSV
    UFUNCTION(BlueprintCallable, Category = "Asset Table")
    bool ImportFromCSV(const FString& FilePath);

    // Update the table with the latest asset data
    UFUNCTION(BlueprintCallable, Category = "Asset Table")
    void UpdateFromAssetManager();
}; 