#include "Assets/CustomAssetTable.h"
#include "Assets/CustomAssetManager.h"
#include "Assets/CustomAssetBase.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

UCustomAssetTable::UCustomAssetTable()
{
    // Set the row structure to FCustomAssetTableRow
    RowStruct = FCustomAssetTableRow::StaticStruct();
}

bool UCustomAssetTable::ExportToCSV(const FString& FilePath) const
{
    FString CSVContent = "AssetId,AssetType,DisplayName,Description,Tags,Version,LastModified,AssetPath\n";

    // Get all rows from the table
    TArray<FName> RowNames = GetRowNames();
    
    // Sort row names for consistent output
    RowNames.Sort([](const FName& A, const FName& B) {
        return A.ToString() < B.ToString();
    });

    // Process each row
    for (const FName& RowName : RowNames)
    {
        const FCustomAssetTableRow* Row = FindRow<FCustomAssetTableRow>(RowName, TEXT(""));
        if (Row)
        {
            // Format the tags as a pipe-separated list
            FString TagsStr;
            for (int32 i = 0; i < Row->Tags.Num(); ++i)
            {
                if (i > 0)
                {
                    TagsStr += "|";
                }
                TagsStr += Row->Tags[i].ToString();
            }

            // Escape any commas in the description
            FString EscapedDescription = Row->Description.ToString();
            EscapedDescription.ReplaceInline(TEXT(","), TEXT("\\,"));
            EscapedDescription.ReplaceInline(TEXT("\n"), TEXT("\\n"));

            // Add the row data to the CSV
            CSVContent += FString::Printf(TEXT("%s,%s,%s,%s,%s,%d,%s,%s\n"),
                *Row->AssetId.ToString(),
                *Row->AssetType.ToString(),
                *Row->DisplayName.ToString(),
                *EscapedDescription,
                *TagsStr,
                Row->Version,
                *Row->LastModified,
                *Row->AssetPath.ToString());
        }
    }

    // Write the CSV file
    return FFileHelper::SaveStringToFile(CSVContent, *FilePath);
}

bool UCustomAssetTable::ImportFromCSV(const FString& FilePath)
{
    // Read the CSV file
    FString CSVContent;
    if (!FFileHelper::LoadFileToString(CSVContent, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load CSV file: %s"), *FilePath);
        return false;
    }

    // Parse the CSV content
    TArray<FString> Lines;
    CSVContent.ParseIntoArrayLines(Lines);

    // Check if the file has at least a header row
    if (Lines.Num() < 1)
    {
        UE_LOG(LogTemp, Error, TEXT("CSV file is empty: %s"), *FilePath);
        return false;
    }

    // Skip the header row
    for (int32 i = 1; i < Lines.Num(); ++i)
    {
        FString Line = Lines[i];
        TArray<FString> Columns;
        Line.ParseIntoArray(Columns, TEXT(","), true);

        // Check if the row has enough columns
        if (Columns.Num() < 8)
        {
            UE_LOG(LogTemp, Warning, TEXT("CSV row %d has insufficient columns, skipping"), i);
            continue;
        }

        // Create a new row
        FCustomAssetTableRow NewRow;
        NewRow.AssetId = FName(*Columns[0]);
        NewRow.AssetType = FName(*Columns[1]);
        NewRow.DisplayName = FText::FromString(Columns[2]);
        
        // Unescape the description
        FString Description = Columns[3];
        Description.ReplaceInline(TEXT("\\,"), TEXT(","));
        Description.ReplaceInline(TEXT("\\n"), TEXT("\n"));
        NewRow.Description = FText::FromString(Description);

        // Parse the tags
        FString TagsStr = Columns[4];
        TArray<FString> TagsArray;
        TagsStr.ParseIntoArray(TagsArray, TEXT("|"), true);
        for (const FString& Tag : TagsArray)
        {
            if (!Tag.IsEmpty())
            {
                NewRow.Tags.Add(FName(*Tag));
            }
        }

        // Parse the version
        NewRow.Version = FCString::Atoi(*Columns[5]);
        
        // Parse the last modified timestamp
        NewRow.LastModified = Columns[6];
        
        // Parse the asset path
        NewRow.AssetPath = FSoftObjectPath(Columns[7]);

        // Add the row to the table
        AddRow(NewRow.AssetId, NewRow);
    }

    return true;
}

void UCustomAssetTable::UpdateFromAssetManager()
{
    // Get the asset manager
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();

    // Get all asset IDs
    TArray<FName> AssetIds = AssetManager.GetAllAssetIds();

    // Process each asset
    for (const FName& AssetId : AssetIds)
    {
        // Try to get the loaded asset
        UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
        
        // Create or update the row
        FCustomAssetTableRow* Row = FindRow<FCustomAssetTableRow>(AssetId, TEXT(""), false);
        if (!Row)
        {
            // Create a new row
            FCustomAssetTableRow NewRow;
            NewRow.AssetId = AssetId;
            
            if (Asset)
            {
                // Fill in the row data from the asset
                NewRow.AssetType = Asset->GetClass()->GetFName();
                NewRow.DisplayName = Asset->DisplayName;
                NewRow.Description = Asset->Description;
                NewRow.Tags = Asset->Tags;
                NewRow.Version = Asset->Version;
                NewRow.LastModified = Asset->LastModified.ToString();
                
                // Get the asset path
                FSoftObjectPath AssetPath;
                if (AssetManager.AssetPathMap.Contains(AssetId))
                {
                    AssetPath = AssetManager.AssetPathMap[AssetId];
                    NewRow.AssetPath = AssetPath;
                }
            }
            
            // Add the row to the table
            AddRow(AssetId, NewRow);
        }
        else if (Asset)
        {
            // Update the existing row
            Row->AssetType = Asset->GetClass()->GetFName();
            Row->DisplayName = Asset->DisplayName;
            Row->Description = Asset->Description;
            Row->Tags = Asset->Tags;
            Row->Version = Asset->Version;
            Row->LastModified = Asset->LastModified.ToString();
            
            // Get the asset path
            if (AssetManager.AssetPathMap.Contains(AssetId))
            {
                Row->AssetPath = AssetManager.AssetPathMap[AssetId];
            }
        }
    }
} 