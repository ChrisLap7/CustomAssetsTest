#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Docking/TabManager.h"
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "Styling/AppStyle.h"
#include "Assets/CustomAssetBase.h"
#include "Assets/CustomAssetBundle.h"
#include "Assets/CustomItemAsset.h"
#include "Assets/CustomCharacterAsset.h"
#include "Editor/CustomAssetManagerCommands.h"

/**
 * Asset data entry for the list view
 */
struct FAssetEntry
{
    /** The asset ID */
    FName AssetId;
    
    /** The asset display name */
    FText DisplayName;
    
    /** The asset description */
    FText Description;
    
    /** Whether the asset is currently loaded */
    bool bIsLoaded;
    
    /** The asset version */
    int32 Version;
    
    /** The asset tags */
    TArray<FName> Tags;
    
    /** The number of dependencies */
    int32 DependencyCount;
    
    /** The asset memory usage in bytes */
    int64 MemoryUsage;
    
    /** Bundles this asset belongs to */
    TArray<FName> Bundles;
    
    /** The asset class type */
    UClass* AssetClass;
    
    /** The asset type */
    FString AssetType;
    
    /** Item asset */
    UCustomItemAsset* ItemAsset = nullptr;
    
    /** Character asset */
    UCustomCharacterAsset* CharacterAsset = nullptr;
    
    FAssetEntry() : bIsLoaded(false), Version(0), DependencyCount(0), MemoryUsage(0), AssetClass(nullptr) {}
    
    /** Get a string listing the bundles this asset belongs to */
    FString GetBundleListString() const
    {
        if (Bundles.Num() == 0)
        {
            return TEXT("None");
        }
        
        FString Result;
        for (int32 i = 0; i < Bundles.Num(); ++i)
        {
            Result += Bundles[i].ToString();
            if (i < Bundles.Num() - 1)
            {
                Result += TEXT(", ");
            }
        }
        return Result;
    }
    
    /** Get a human-readable asset type name */
    FText GetAssetTypeName() const
    {
        if (!AssetClass)
        {
            return FText::FromString(TEXT("Unknown"));
        }
        else if (AssetClass == UCustomItemAsset::StaticClass())
        {
            return FText::FromString(TEXT("Item"));
        }
        else if (AssetClass == UCustomCharacterAsset::StaticClass())
        {
            return FText::FromString(TEXT("Character"));
        }
        else if (AssetClass == UCustomAssetBundle::StaticClass())
        {
            return FText::FromString(TEXT("Bundle"));
        }
        else if (AssetClass == UCustomAssetBase::StaticClass())
        {
            return FText::FromString(TEXT("Base Asset"));
        }
        else
        {
            return FText::FromString(AssetClass->GetName().Replace(TEXT("UCustom"), TEXT("")).Replace(TEXT("Asset"), TEXT("")));
        }
    }
    
    /** Get color for the asset type */
    FSlateColor GetAssetTypeColor() const
    {
        if (!AssetClass)
        {
            return FLinearColor::White;
        }
        else if (AssetClass == UCustomItemAsset::StaticClass())
        {
            return FLinearColor(1.0f, 0.8f, 0.2f); // Gold for items
        }
        else if (AssetClass == UCustomCharacterAsset::StaticClass())
        {
            return FLinearColor(0.2f, 0.8f, 1.0f); // Blue for characters
        }
        else if (AssetClass == UCustomAssetBundle::StaticClass())
        {
            return FLinearColor(0.4f, 0.8f, 0.4f); // Green for bundles
        }
        else
        {
            return FLinearColor(0.4f, 0.4f, 1.0f); // Blue for base assets
        }
    }
};

/**
 * Bundle data entry for the list view
 */
struct FBundleEntry
{
    /** Reference to the actual bundle */
    UCustomAssetBundle* Bundle;
};

/**
 * Simple asset list item for popup dialogs
 */
struct FAssetListItem
{
    /** The asset ID */
    FName AssetId;
    
    /** The asset display name */
    FText DisplayName;
};

/**
 * Dependency entry for the dependency view
 */
struct FDependencyEntry
{
    /** The asset ID */
    FName AssetId;
    
    /** The asset display name */
    FText DisplayName;
    
    /** Whether this is a hard dependency */
    bool bIsHardDependency;
    
    /** The dependency type */
    FName DependencyType;
};

/**
 * Custom asset manager editor window
 */
class SCustomAssetManagerEditorWindow : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCustomAssetManagerEditorWindow) 
        : _OwnerWindow()
    {}
        SLATE_ARGUMENT(TSharedPtr<SWindow>, OwnerWindow)
    SLATE_END_ARGS()

    /** Constructor */
    SCustomAssetManagerEditorWindow();
    
    /** Destructor */
    ~SCustomAssetManagerEditorWindow();

    /** Constructs the widget */
    void Construct(const FArguments& InArgs);

    /**
     * Creates the window and opens it
     */
    static void OpenWindow();

private:
    /** Window title */
    FText WindowTitle;

    /** Commands for the tab menu */
    TSharedPtr<FUICommandList> CommandList;

    /** Tab manager for the window */
    TSharedPtr<FTabManager> TabManager;

    /** Asset list view */
    TSharedPtr<SListView<TSharedPtr<FAssetEntry>>> AssetListView;

    /** Bundle list view */
    TSharedPtr<SListView<TSharedPtr<FBundleEntry>>> BundleListView;

    /** Dependency list view */
    TSharedPtr<SListView<TSharedPtr<FDependencyEntry>>> DependencyListView;

    /** Asset data */
    TArray<TSharedPtr<FAssetEntry>> AssetEntries;

    /** Filtered asset data */
    TArray<TSharedPtr<FAssetEntry>> FilteredAssetEntries;

    /** Bundle data */
    TArray<TSharedPtr<FBundleEntry>> BundleEntries;

    /** Dependency data */
    TArray<TSharedPtr<FDependencyEntry>> DependencyEntries;

    /** Currently selected asset */
    FName SelectedAssetId;

    /** Currently selected bundle */
    FName SelectedBundleId;

    /** View mode for dependencies */
    bool bShowingDependents;

    /** Search text for asset list */
    FString AssetSearchString;

    /** Current type filter */
    FString TypeFilter;

    /** Current rarity filter */
    FString RarityFilter;

    /** Current bundle filter */
    FName BundleFilter;

    /** Search box for asset list */
    TSharedPtr<SSearchBox> AssetSearchBox;

    /** Creates the menu bar */
    TSharedRef<SWidget> CreateMenuBar();

    /** Creates the toolbar */
    TSharedRef<SWidget> CreateToolbar();

    /** Fill the file menu */
    void FillFileMenu(FMenuBuilder& MenuBuilder);

    /** Fill the export menu */
    void FillExportMenu(FMenuBuilder& MenuBuilder);

    /** Fill the import menu */
    void FillImportMenu(FMenuBuilder& MenuBuilder);

    /** Close the window */
    void CloseWindow();

    /** Creates the asset tab */
    TSharedRef<SDockTab> SpawnAssetTab(const FSpawnTabArgs& Args);

    /** Creates the bundle tab */
    TSharedRef<SDockTab> SpawnBundleTab(const FSpawnTabArgs& Args);

    /** Creates the memory tab */
    TSharedRef<SDockTab> SpawnMemoryTab(const FSpawnTabArgs& Args);

    /** Creates the dependency tab */
    TSharedRef<SDockTab> SpawnDependencyTab(const FSpawnTabArgs& Args);

    /** Refreshes the asset list */
    void RefreshAssetList();

    /** Refreshes the bundle list */
    void RefreshBundleList();

    /** Called when the refresh asset list button is clicked */
    FReply OnRefreshAssetListClicked();

    /** Called when the refresh bundle list button is clicked */
    FReply OnRefreshBundleListClicked();

    /** Updates the dependency list for the selected asset */
    void UpdateDependencyList();

    /** Called when an asset is selected in the list */
    void OnAssetSelectionChanged(TSharedPtr<FAssetEntry> SelectedItem, ESelectInfo::Type SelectInfo);

    /** Called when a bundle is selected in the list */
    void OnBundleSelectionChanged(TSharedPtr<FBundleEntry> SelectedItem, ESelectInfo::Type SelectInfo);

    /** Called when the "Export Assets to CSV" button is clicked */
    FReply OnExportAssetsToCSVClicked();

    /** Called when the "Export Memory Usage to CSV" button is clicked */
    FReply OnExportMemoryUsageToCSVClicked();

    /** Called when the "Export Dependency Graph" button is clicked */
    FReply OnExportDependencyGraphClicked();

    /** Called when the "Import Assets from CSV" button is clicked */
    FReply OnImportAssetsFromCSVClicked();

    /** Function to import assets from a CSV file */
    bool ImportAssetsFromCSV(const FString& FilePath);

    /** Called when the "Load Asset" button is clicked */
    FReply OnLoadAssetClicked();

    /** Called when the "Unload Asset" button is clicked */
    FReply OnUnloadAssetClicked();

    /** Called when the "Add to Bundle" button is clicked */
    FReply OnAddAssetToBundleClicked();

    /** Called when the "Create Bundle" button is clicked */
    FReply OnCreateBundleClicked();

    /** Called when the "Load Bundle" button is clicked */
    FReply OnLoadBundleClicked();

    /** Called when the "Unload Bundle" button is clicked */
    FReply OnUnloadBundleClicked();

    /** Called when the "Toggle Dependency View" button is clicked */
    FReply OnToggleDependencyViewClicked();

    /** Called to generate a row for the asset list */
    TSharedRef<ITableRow> GenerateAssetRow(TSharedPtr<FAssetEntry> Item, const TSharedRef<STableViewBase>& OwnerTable);

    /** Called to generate a row for the bundle list */
    TSharedRef<ITableRow> GenerateBundleRow(TSharedPtr<FBundleEntry> Item, const TSharedRef<STableViewBase>& OwnerTable);

    /** Called to generate a row for the dependency list */
    TSharedRef<ITableRow> GenerateDependencyRow(TSharedPtr<FDependencyEntry> Item, const TSharedRef<STableViewBase>& OwnerTable);

    /** Formats a memory size for display */
    FText FormatMemorySize(int64 MemoryInBytes) const;

    /** Check if an asset is selected */
    bool IsAssetSelected() const;

    /** Check if a bundle is selected */
    bool IsBundleSelected() const;

    /** Get the current memory usage text */
    FText GetCurrentMemoryUsageText() const;

    /** Get the memory threshold text */
    FText GetMemoryThresholdText() const;

    /** Get the memory policy text */
    FText GetMemoryPolicyText() const;

    /** Get the dependency header text */
    FText GetDependencyHeaderText() const;

    /** Get the currently selected asset */
    UCustomAssetBase* GetSelectedAsset() const;

    /** Called to remove an asset from a bundle */
    FReply OnRemoveAssetFromBundleClicked();
    
    /** Shows a dialog to remove an asset from one of its bundles */
    void ShowRemoveFromBundleDialog(const FName& AssetId);
    
    /** Shows a dialog to rename a bundle */
    void ShowRenameBundleDialog(UCustomAssetBundle* Bundle);

    /** Shows a dialog to confirm deletion of a bundle */
    void ShowDeleteBundleDialog(UCustomAssetBundle* Bundle);

    /** Called when the search text changes */
    void OnAssetSearchTextChanged(const FText& NewText);
    
    /** Called when the type filter changes */
    void OnTypeFilterChanged(FString NewFilter);
    
    /** Called when the rarity filter changes */
    void OnRarityFilterChanged(FString NewFilter);
    
    /** Called when the bundle filter changes */
    void OnBundleFilterChanged(FName NewFilter);
    
    /** Called when the type filter changes */
    void OnTypeFilterChanged_ComboBox(TSharedPtr<FString> NewValue);
    
    /** Called when the rarity filter changes */
    void OnRarityFilterChanged_ComboBox(TSharedPtr<FString> NewValue);
    
    /** Called when the bundle filter changes */
    void OnBundleFilterChanged_ComboBox(TSharedPtr<FBundleEntry> NewValue);
    
    /** Called when the reset filters button is clicked */
    FReply OnResetFiltersClicked();
    
    /** Apply current filters to the asset list */
    void ApplyFilters();

    /** Creates the filter widgets for the asset tab */
    TSharedRef<SWidget> CreateAssetFilterWidgets();

    // Helper functions
    FLinearColor GetAssetTypeColor(const FString& AssetType) const;
};

// Comment out the conflicting module class - we already have FCustomAssetEditorModule
/*
class FCustomAssetManagerEditorModule : public IModuleInterface
{
public:
    // IModuleInterface implementation 
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    // Registers the custom asset manager commands 
    void RegisterCustomAssetManagerCommands();
    
    // Called when the custom asset manager menu item is clicked 
    void OnOpenAssetManagerWindow();

    // Command list for the module 
    TSharedPtr<FUICommandList> CommandList;
};
*/ 