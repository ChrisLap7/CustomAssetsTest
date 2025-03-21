#include "Editor/CustomAssetManagerEditorWindow.h"
#include "Assets/CustomAssetManager.h"
#include "Assets/CustomAssetBase.h"
#include "Assets/CustomAssetBundle.h"
#include "Assets/CustomItemAsset.h"
#include "Assets/CustomCharacterAsset.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Editor/CustomAssetManagerCommands.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "AssetRegistry/AssetRegistryModule.h"

// Forward declarations
EItemQuality StringToItemQuality(const FString& QualityStr);
FString GetEnumValueString(const EItemQuality Quality);
FString GetEnumValueString(const ECharacterClass CharClass);

#define LOCTEXT_NAMESPACE "CustomAssetManager"

// Define static tab ID constants
static const FName TAB_ID_ASSETS = TEXT("Assets");
static const FName TAB_ID_BUNDLES = TEXT("Bundles");
static const FName TAB_ID_MEMORY = TEXT("Memory");
static const FName TAB_ID_DEPENDENCIES = TEXT("Dependencies");

// Window creation
void SCustomAssetManagerEditorWindow::OpenWindow()
{
    TSharedRef<SWindow> Window = SNew(SWindow)
        .Title(LOCTEXT("CustomAssetManagerWindowTitle", "Custom Asset Manager"))
        .ClientSize(FVector2D(1024, 768))
        .SupportsMaximize(true)
        .SupportsMinimize(true);

    // Create the window content without passing any parameters
    Window->SetContent(
        SNew(SCustomAssetManagerEditorWindow)
    );

    FSlateApplication::Get().AddWindow(Window);
    
    UE_LOG(LogTemp, Display, TEXT("Custom Asset Manager Window opened"));
}

SCustomAssetManagerEditorWindow::SCustomAssetManagerEditorWindow()
{
    UE_LOG(LogTemp, Display, TEXT("Creating Custom Asset Manager Window"));
}

SCustomAssetManagerEditorWindow::~SCustomAssetManagerEditorWindow()
{
    UE_LOG(LogTemp, Display, TEXT("Destroying Custom Asset Manager Window"));
    
    // Make sure to unregister all tab spawners when the window is destroyed
    if (FGlobalTabmanager::Get()->HasTabSpawner(TAB_ID_ASSETS))
    {
        FGlobalTabmanager::Get()->UnregisterTabSpawner(TAB_ID_ASSETS);
        UE_LOG(LogTemp, Display, TEXT("Unregistered Assets tab spawner in destructor"));
    }
    
    if (FGlobalTabmanager::Get()->HasTabSpawner(TAB_ID_BUNDLES))
    {
        FGlobalTabmanager::Get()->UnregisterTabSpawner(TAB_ID_BUNDLES);
        UE_LOG(LogTemp, Display, TEXT("Unregistered Bundles tab spawner in destructor"));
    }
    
    if (FGlobalTabmanager::Get()->HasTabSpawner(TAB_ID_MEMORY))
    {
        FGlobalTabmanager::Get()->UnregisterTabSpawner(TAB_ID_MEMORY);
        UE_LOG(LogTemp, Display, TEXT("Unregistered Memory tab spawner in destructor"));
    }
    
    if (FGlobalTabmanager::Get()->HasTabSpawner(TAB_ID_DEPENDENCIES))
    {
        FGlobalTabmanager::Get()->UnregisterTabSpawner(TAB_ID_DEPENDENCIES);
        UE_LOG(LogTemp, Display, TEXT("Unregistered Dependencies tab spawner in destructor"));
    }
}

void SCustomAssetManagerEditorWindow::Construct(const FArguments& InArgs)
{
    WindowTitle = FText::FromString("Custom Asset Manager");
    CommandList = MakeShareable(new FUICommandList);
    
    // Initialize filter properties
    AssetSearchString.Empty();
    TypeFilter.Empty();
    RarityFilter.Empty();
    BundleFilter = FName(NAME_None);

    // Create tab manager
    const TSharedRef<SDockTab> MajorTab = SNew(SDockTab).TabRole(ETabRole::MajorTab);
    TabManager = FGlobalTabmanager::Get()->NewTabManager(MajorTab);
    
    // Create the asset list view
    AssetListView = SNew(SListView<TSharedPtr<FAssetEntry>>)
        .ListItemsSource(&FilteredAssetEntries) // Use filtered entries instead of all entries
        .OnGenerateRow(this, &SCustomAssetManagerEditorWindow::GenerateAssetRow)
        .OnSelectionChanged(this, &SCustomAssetManagerEditorWindow::OnAssetSelectionChanged)
        .SelectionMode(ESelectionMode::Multi); // Allow multiple selection
    
    // Create the bundle list view
    BundleListView = SNew(SListView<TSharedPtr<FBundleEntry>>)
        .ListItemsSource(&BundleEntries)
        .OnGenerateRow(this, &SCustomAssetManagerEditorWindow::GenerateBundleRow)
        .OnSelectionChanged(this, &SCustomAssetManagerEditorWindow::OnBundleSelectionChanged);
    
    // Instead of creating our own tab manager, use the global one directly
    static const FName AssetManagerTabsApp = FName("AssetManagerTabs");
    
    // Unregister existing tab spawners if they exist to prevent exceptions when reopening
    if (FGlobalTabmanager::Get()->HasTabSpawner(TAB_ID_ASSETS))
    {
        FGlobalTabmanager::Get()->UnregisterTabSpawner(TAB_ID_ASSETS);
        UE_LOG(LogTemp, Log, TEXT("Unregistered existing Assets tab spawner"));
    }
    
    if (FGlobalTabmanager::Get()->HasTabSpawner(TAB_ID_BUNDLES))
    {
        FGlobalTabmanager::Get()->UnregisterTabSpawner(TAB_ID_BUNDLES);
        UE_LOG(LogTemp, Log, TEXT("Unregistered existing Bundles tab spawner"));
    }
    
    // Make sure the other tabs are unregistered too
    if (FGlobalTabmanager::Get()->HasTabSpawner(TAB_ID_MEMORY))
    {
        FGlobalTabmanager::Get()->UnregisterTabSpawner(TAB_ID_MEMORY);
        UE_LOG(LogTemp, Log, TEXT("Unregistered existing Memory tab spawner"));
    }
    
    if (FGlobalTabmanager::Get()->HasTabSpawner(TAB_ID_DEPENDENCIES))
    {
        FGlobalTabmanager::Get()->UnregisterTabSpawner(TAB_ID_DEPENDENCIES);
        UE_LOG(LogTemp, Log, TEXT("Unregistered existing Dependencies tab spawner"));
    }
    
    // Register tab spawners
    FGlobalTabmanager::Get()->RegisterTabSpawner(
        TAB_ID_ASSETS, 
        FOnSpawnTab::CreateLambda([this](const FSpawnTabArgs& Args) { return this->SpawnAssetTab(Args); }))
        .SetDisplayName(LOCTEXT("AssetsTabTitle", "Assets"))
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.TabIcon"));
    
    FGlobalTabmanager::Get()->RegisterTabSpawner(
        TAB_ID_BUNDLES, 
        FOnSpawnTab::CreateLambda([this](const FSpawnTabArgs& Args) { return this->SpawnBundleTab(Args); }))
        .SetDisplayName(LOCTEXT("BundlesTabTitle", "Bundles"))
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.TabIcon.Starred"));
    
    FGlobalTabmanager::Get()->RegisterTabSpawner(
        TAB_ID_MEMORY, 
        FOnSpawnTab::CreateLambda([this](const FSpawnTabArgs& Args) { return this->SpawnMemoryTab(Args); }))
        .SetDisplayName(LOCTEXT("MemoryTabTitle", "Memory Usage"))
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Performance"));
    
    FGlobalTabmanager::Get()->RegisterTabSpawner(
        TAB_ID_DEPENDENCIES, 
        FOnSpawnTab::CreateLambda([this](const FSpawnTabArgs& Args) { return this->SpawnDependencyTab(Args); }))
        .SetDisplayName(LOCTEXT("DependenciesTabTitle", "Dependencies"))
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Graph"));

    // Create tab layout for the window
    const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("CustomAssetManagerLayout")
        ->AddArea
        (
            FTabManager::NewPrimaryArea()
            ->SetOrientation(Orient_Vertical)
            ->Split
            (
                FTabManager::NewStack()
                ->AddTab("Assets", ETabState::OpenedTab)
                ->AddTab("Bundles", ETabState::OpenedTab)
                ->AddTab("Memory", ETabState::OpenedTab)
                ->AddTab("Dependencies", ETabState::OpenedTab)
                ->SetForegroundTab(FName("Assets"))
            )
        );
        
    // Set TabManager to the global manager
    TabManager = FGlobalTabmanager::Get();
    
    // Create child slot with the tab manager
    ChildSlot
    [
        SNew(SVerticalBox)
        
        // Menu bar
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            CreateMenuBar()
        ]
        
        // Toolbar
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            CreateToolbar()
        ]
        
        // Main content area with tabs
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        [
            FGlobalTabmanager::Get()->RestoreFrom(Layout, TSharedPtr<SWindow>()).ToSharedRef()
        ]
    ];
    
    // Refresh the asset list
    RefreshAssetList();
    RefreshBundleList();
}

TSharedRef<SWidget> SCustomAssetManagerEditorWindow::CreateMenuBar()
{
    FMenuBarBuilder MenuBarBuilder = FMenuBarBuilder(CommandList);
    
    // File menu
    MenuBarBuilder.AddPullDownMenu(
        LOCTEXT("FileMenu", "File"),
        LOCTEXT("FileMenuTooltip", "File operations"),
        FNewMenuDelegate::CreateRaw(this, &SCustomAssetManagerEditorWindow::FillFileMenu)
    );
    
    // Export menu
    MenuBarBuilder.AddPullDownMenu(
        LOCTEXT("ExportMenu", "Export"),
        LOCTEXT("ExportMenuTooltip", "Export options"),
        FNewMenuDelegate::CreateRaw(this, &SCustomAssetManagerEditorWindow::FillExportMenu)
    );
    
    return MenuBarBuilder.MakeWidget();
}

TSharedRef<SWidget> SCustomAssetManagerEditorWindow::CreateToolbar()
{
    FToolBarBuilder ToolBarBuilder(CommandList, FMultiBoxCustomization::None);
    
    ToolBarBuilder.BeginSection("General");
    {
        // Create the refresh button
        ToolBarBuilder.AddToolBarButton(
            FUIAction(FExecuteAction::CreateLambda([this]() { this->OnRefreshAssetListClicked(); })),
            NAME_None,
            LOCTEXT("RefreshButton", "Refresh"),
            LOCTEXT("RefreshButtonTooltip", "Refresh the asset and bundle lists"),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Refresh")
        );
    }
    ToolBarBuilder.EndSection();
    
    return ToolBarBuilder.MakeWidget();
}

void SCustomAssetManagerEditorWindow::FillFileMenu(FMenuBuilder& MenuBuilder)
{
    // Add the import menu section
    MenuBuilder.BeginSection("Import", LOCTEXT("ImportSection", "Import"));
    MenuBuilder.AddSubMenu(
        LOCTEXT("Import", "Import"),
        LOCTEXT("ImportTooltip", "Import assets from external files"),
        FNewMenuDelegate::CreateRaw(this, &SCustomAssetManagerEditorWindow::FillImportMenu)
    );
    MenuBuilder.EndSection();
    
    // Add the export menu section
    MenuBuilder.BeginSection("Export", LOCTEXT("ExportSection", "Export"));
    MenuBuilder.AddSubMenu(
        LOCTEXT("Export", "Export"),
        LOCTEXT("ExportTooltip", "Export assets to external files"),
        FNewMenuDelegate::CreateRaw(this, &SCustomAssetManagerEditorWindow::FillExportMenu)
    );
    MenuBuilder.EndSection();
    
    // Add the close menu section
    MenuBuilder.BeginSection("FileSection", LOCTEXT("FileSection", "File"));
    {
        FUIAction CloseAction;
        CloseAction.ExecuteAction = FExecuteAction::CreateSP(this, &SCustomAssetManagerEditorWindow::CloseWindow);
        
        MenuBuilder.AddMenuEntry(
            LOCTEXT("CloseWindow", "Close Window"),
            LOCTEXT("CloseWindowTooltip", "Closes the custom asset manager window"),
            FSlateIcon(),
            CloseAction
        );
    }
    MenuBuilder.EndSection();
}

void SCustomAssetManagerEditorWindow::FillExportMenu(FMenuBuilder& MenuBuilder)
{
    MenuBuilder.BeginSection("Export", LOCTEXT("ExportMenuSection", "Export"));
    {
        // Export assets to CSV
        MenuBuilder.AddMenuEntry(
            LOCTEXT("ExportAssetsToCSV", "Export Assets to CSV"),
            LOCTEXT("ExportAssetsToCSVTooltip", "Export asset data to a CSV file"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateLambda([this]() { this->OnExportAssetsToCSVClicked(); }))
        );
        
        // Export memory usage to CSV
        MenuBuilder.AddMenuEntry(
            LOCTEXT("ExportMemoryUsageToCSV", "Export Memory Usage to CSV"),
            LOCTEXT("ExportMemoryUsageToCSVTooltip", "Export memory usage data to a CSV file"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateLambda([this]() { this->OnExportMemoryUsageToCSVClicked(); }))
        );
        
        // Export dependency graph
        MenuBuilder.AddMenuEntry(
            LOCTEXT("ExportDependencyGraph", "Export Dependency Graph"),
            LOCTEXT("ExportDependencyGraphTooltip", "Export dependency relationships to a graph file"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateLambda([this]() { this->OnExportDependencyGraphClicked(); }))
        );
    }
    MenuBuilder.EndSection();
}

void SCustomAssetManagerEditorWindow::FillImportMenu(FMenuBuilder& MenuBuilder)
{
    // Add the "Import Assets from CSV" menu entry
    MenuBuilder.AddMenuEntry(
        LOCTEXT("ImportAssetsFromCSV", "Import Assets from CSV"),
        LOCTEXT("ImportAssetsFromCSVTooltip", "Import assets from a CSV file"),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateLambda([this]() { this->OnImportAssetsFromCSVClicked(); }))
    );
}

void SCustomAssetManagerEditorWindow::CloseWindow()
{
    TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
    if (ParentWindow.IsValid())
    {
        ParentWindow->RequestDestroyWindow();
    }
}

TSharedRef<SDockTab> SCustomAssetManagerEditorWindow::SpawnAssetTab(const FSpawnTabArgs& Args)
{
    // Create the filter widgets
    TSharedRef<SWidget> FilterWidgets = CreateAssetFilterWidgets();
    
    return SNew(SDockTab)
        .TabRole(ETabRole::PanelTab)
        .Label(FText::FromString("Assets"))
        [
            SNew(SVerticalBox)
            
            // Add the filter widgets
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(5)
            [
                FilterWidgets
            ]
            
            // Add the asset list
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(5)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                [
                    SAssignNew(AssetListView, SListView<TSharedPtr<FAssetEntry>>)
                    .ItemHeight(24)
                    .ListItemsSource(&FilteredAssetEntries)
                    .OnGenerateRow(this, &SCustomAssetManagerEditorWindow::GenerateAssetRow)
                    .OnSelectionChanged(this, &SCustomAssetManagerEditorWindow::OnAssetSelectionChanged)
                    .SelectionMode(ESelectionMode::Multi)
                    .HeaderRow
                    (
                        SNew(SHeaderRow)
                        + SHeaderRow::Column("AssetId")
                        .DefaultLabel(FText::FromString("Asset ID"))
                        .FillWidth(0.2f)

                        + SHeaderRow::Column("Type")
                        .DefaultLabel(FText::FromString("Type"))
                        .FillWidth(0.1f)

                        + SHeaderRow::Column("DisplayName")
                        .DefaultLabel(FText::FromString("Display Name"))
                        .FillWidth(0.2f)

                        + SHeaderRow::Column("Loaded")
                        .DefaultLabel(FText::FromString("Loaded"))
                        .FillWidth(0.1f)

                        + SHeaderRow::Column("Memory")
                        .DefaultLabel(FText::FromString("Memory"))
                        .FillWidth(0.1f)

                        + SHeaderRow::Column("Version")
                        .DefaultLabel(FText::FromString("Version"))
                        .FillWidth(0.1f)

                        + SHeaderRow::Column("Bundles")
                        .DefaultLabel(FText::FromString("Bundles"))
                        .FillWidth(0.15f)
                    )
                ]
            ]
            
            // Add action buttons
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(5)
            [
                SNew(SHorizontalBox)
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(5, 0)
                [
                    SNew(SButton)
                    .Text(FText::FromString("Refresh"))
                    .ToolTipText(FText::FromString("Refresh the asset list"))
                    .OnClicked(this, &SCustomAssetManagerEditorWindow::OnRefreshAssetListClicked)
                ]
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(5, 0)
                [
                    SNew(SButton)
                    .Text(FText::FromString("Load Selected"))
                    .ToolTipText(FText::FromString("Load the selected asset"))
                    .OnClicked(this, &SCustomAssetManagerEditorWindow::OnLoadAssetClicked)
                    .IsEnabled(this, &SCustomAssetManagerEditorWindow::IsAssetSelected)
                ]
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(5, 0)
                [
                    SNew(SButton)
                    .Text(FText::FromString("Unload Selected"))
                    .ToolTipText(FText::FromString("Unload the selected asset"))
                    .OnClicked(this, &SCustomAssetManagerEditorWindow::OnUnloadAssetClicked)
                    .IsEnabled(this, &SCustomAssetManagerEditorWindow::IsAssetSelected)
                ]
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(5, 0)
                [
                    SNew(SButton)
                    .Text(FText::FromString("Add to Bundle"))
                    .ToolTipText(FText::FromString("Add the selected asset to a bundle"))
                    .OnClicked(this, &SCustomAssetManagerEditorWindow::OnAddAssetToBundleClicked)
                    .IsEnabled(this, &SCustomAssetManagerEditorWindow::IsAssetSelected)
                ]
                
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(5, 0)
                [
                    SNew(SBox)
                    .Visibility_Lambda([this]() {
                        UCustomAssetBase* Asset = GetSelectedAsset();
                        if (!Asset) return EVisibility::Collapsed;
                        
                        TSharedPtr<FAssetEntry> SelectedEntry;
                        for (TSharedPtr<FAssetEntry> Entry : AssetEntries) {
                            if (Entry->AssetId == SelectedAssetId) {
                                SelectedEntry = Entry;
                                break;
                            }
                        }
                        
                        return SelectedEntry.IsValid() && SelectedEntry->Bundles.Num() > 0 
                            ? EVisibility::Visible 
                            : EVisibility::Collapsed;
                    })
                    [
                        SNew(SButton)
                        .ContentPadding(FMargin(5, 2))
                        .Text(FText::FromString("Remove"))
                        .ToolTipText(FText::FromString("Remove this asset from a bundle"))
                        .OnClicked_Lambda([this]()
                        {
                            ShowRemoveFromBundleDialog(SelectedAssetId);
                            return FReply::Handled();
                        })
                    ]
                ]
            ]
        ];
}

TSharedRef<SDockTab> SCustomAssetManagerEditorWindow::SpawnBundleTab(const FSpawnTabArgs& Args)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::PanelTab)
        [
            SNew(SVerticalBox)
            +SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0, 0, 0, 4))
            [
                SNew(SHorizontalBox)
                +SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("BundleListHeader", "Asset Bundles"))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                ]
                +SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(FMargin(4, 0, 0, 0))
                [
                    SNew(SButton)
                    .Text(LOCTEXT("CreateBundle", "Create Bundle"))
                    .OnClicked(this, &SCustomAssetManagerEditorWindow::OnCreateBundleClicked)
                ]
            ]
            +SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0, 0, 0, 8))
            [
                SNew(STextBlock)
                .Text(LOCTEXT("BundleLocationInfo", "Bundles are saved to '/Game/Bundles' and can be accessed in your code with UCustomAssetManager::Get().GetBundleById()"))
                .ColorAndOpacity(FLinearColor(0.5f, 0.5f, 1.0f, 1.0f))
            ]
            +SVerticalBox::Slot()
            .FillHeight(1.0f)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                [
                    SAssignNew(BundleListView, SListView<TSharedPtr<FBundleEntry>>)
                    .ItemHeight(28)
                    .ListItemsSource(&BundleEntries)
                    .OnGenerateRow(this, &SCustomAssetManagerEditorWindow::GenerateBundleRow)
                    .OnSelectionChanged(this, &SCustomAssetManagerEditorWindow::OnBundleSelectionChanged)
                    .SelectionMode(ESelectionMode::Single)
                    .HeaderRow
                    (
                        SNew(SHeaderRow)
                        + SHeaderRow::Column("BundleId")
                        .DefaultLabel(LOCTEXT("BundleIdColumn", "Bundle ID"))
                        .FillWidth(0.2f)

                        + SHeaderRow::Column("DisplayName")
                        .DefaultLabel(LOCTEXT("DisplayNameColumn", "Display Name"))
                        .FillWidth(0.2f)

                        + SHeaderRow::Column("Assets")
                        .DefaultLabel(LOCTEXT("AssetsColumn", "Assets"))
                        .FillWidth(0.2f)

                        + SHeaderRow::Column("Status")
                        .DefaultLabel(LOCTEXT("StatusColumn", "Status"))
                        .FillWidth(0.15f)

                        + SHeaderRow::Column("Memory")
                        .DefaultLabel(LOCTEXT("MemoryColumn", "Memory"))
                        .FillWidth(0.1f)
                        
                        + SHeaderRow::Column("Actions")
                        .DefaultLabel(LOCTEXT("ActionsColumn", "Actions"))
                        .FillWidth(0.15f)
                    )
                ]
            ]
        ];
}

TSharedRef<SDockTab> SCustomAssetManagerEditorWindow::SpawnMemoryTab(const FSpawnTabArgs& Args)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::PanelTab)
        [
            SNew(SVerticalBox)
            +SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0, 0, 0, 4))
            [
                SNew(STextBlock)
                .Text(LOCTEXT("MemoryUsageHeader", "Memory Usage Analysis"))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
            ]
            +SVerticalBox::Slot()
            .FillHeight(1.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("MemoryUsagePlace", "Memory usage analysis will be displayed here"))
            ]
        ];
}

TSharedRef<SDockTab> SCustomAssetManagerEditorWindow::SpawnDependencyTab(const FSpawnTabArgs& Args)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::PanelTab)
        [
            SNew(SVerticalBox)
            +SVerticalBox::Slot()
            .AutoHeight()
            .Padding(FMargin(0, 0, 0, 4))
            [
                SNew(STextBlock)
                .Text(LOCTEXT("DependencyHeader", "Asset Dependencies"))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
            ]
            +SVerticalBox::Slot()
            .FillHeight(1.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("DependencyPlace", "Dependency graph will be displayed here"))
            ]
        ];
}

void SCustomAssetManagerEditorWindow::RefreshAssetList()
{
    // Get the asset manager
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();

    // Get all asset IDs, both loaded and unloaded
    TArray<FName> AssetIds = AssetManager.GetAllAssetIds();
    
    // Get all loaded assets
    TArray<UCustomAssetBase*> LoadedAssets = AssetManager.GetAllLoadedAssets();
    
    // Clear existing entries
    AssetEntries.Empty();
    FilteredAssetEntries.Empty();
    
    // First add loaded assets
    for (UCustomAssetBase* Asset : LoadedAssets)
    {
        if (Asset)
        {
            TSharedPtr<FAssetEntry> Entry = MakeShared<FAssetEntry>();
            Entry->AssetId = Asset->AssetId;
            Entry->DisplayName = Asset->DisplayName;
            Entry->Description = Asset->Description;
            Entry->bIsLoaded = true;
            Entry->Version = Asset->Version;
            Entry->Tags = Asset->Tags;
            Entry->DependencyCount = Asset->Dependencies.Num();
            Entry->AssetClass = Asset->GetClass();
            
            // Set asset type based on class
            UCustomItemAsset* ItemAsset = Cast<UCustomItemAsset>(Asset);
            if (ItemAsset)
            {
                Entry->ItemAsset = ItemAsset;
                Entry->AssetType = TEXT("Item");
            }
            else
            {
                UCustomCharacterAsset* CharAsset = Cast<UCustomCharacterAsset>(Asset);
                if (CharAsset)
                {
                    Entry->CharacterAsset = CharAsset;
                    Entry->AssetType = TEXT("Character");
                }
                else
                {
                    Entry->AssetType = TEXT("Asset");
                }
            }
            
            // Get memory usage
            Entry->MemoryUsage = AssetManager.EstimateAssetMemoryUsage(Asset);
            
            // Find which bundles this asset is in
            TArray<UCustomAssetBundle*> ContainingBundles = AssetManager.GetAllBundlesContainingAsset(Asset->AssetId);
            for (UCustomAssetBundle* Bundle : ContainingBundles)
            {
                if (Bundle)
                {
                    Entry->Bundles.Add(Bundle->BundleId);
                }
            }
            
            AssetEntries.Add(Entry);
        }
    }
    
    // Then add unloaded assets (which are in the AssetIds list but not in LoadedAssets)
    for (const FName& AssetId : AssetIds)
    {
        // Skip if already added as a loaded asset
        bool bAlreadyAdded = false;
        for (const TSharedPtr<FAssetEntry>& Entry : AssetEntries)
        {
            if (Entry->AssetId == AssetId)
            {
                bAlreadyAdded = true;
                break;
            }
        }
        
        if (bAlreadyAdded)
        {
            continue;
        }
        
        // Add as an unloaded asset
        TSharedPtr<FAssetEntry> Entry = MakeShared<FAssetEntry>();
        Entry->AssetId = AssetId;
        Entry->DisplayName = FText::FromName(AssetId); // Use ID as display name for unloaded assets
        Entry->Description = FText::FromString("(Asset not loaded)");
        Entry->bIsLoaded = false;
        Entry->Version = 0;
        Entry->DependencyCount = 0;
        Entry->AssetClass = UCustomAssetBase::StaticClass(); // Default class
        Entry->AssetType = TEXT("Unknown");
        Entry->MemoryUsage = 0;
        
        // Find which bundles this asset is in
        TArray<UCustomAssetBundle*> ContainingBundles = AssetManager.GetAllBundlesContainingAsset(AssetId);
        for (UCustomAssetBundle* Bundle : ContainingBundles)
        {
            if (Bundle)
            {
                Entry->Bundles.Add(Bundle->BundleId);
            }
        }
        
        AssetEntries.Add(Entry);
    }
    
    // Apply any filters
    ApplyFilters();
    
    // Update the list view
    if (AssetListView.IsValid())
    {
        AssetListView->RequestListRefresh();
    }
}

FReply SCustomAssetManagerEditorWindow::OnRefreshAssetListClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("SCustomAssetManagerEditorWindow: OnRefreshAssetListClicked - Refresh button clicked"));
    
    // Refresh the asset list
    RefreshAssetList();
    
    return FReply::Handled();
}

void SCustomAssetManagerEditorWindow::RefreshBundleList()
{
    BundleEntries.Empty();
    
    // Get the asset manager
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    
    // Get all bundles
    TArray<UCustomAssetBundle*> Bundles;
    AssetManager.GetAllBundles(Bundles);
    
    // Add to list
    for (UCustomAssetBundle* Bundle : Bundles)
    {
        TSharedPtr<FBundleEntry> Entry = MakeShared<FBundleEntry>();
        Entry->Bundle = Bundle;
        
        BundleEntries.Add(Entry);
    }
    
    // Refresh the list view
    BundleListView->RequestListRefresh();
}

FReply SCustomAssetManagerEditorWindow::OnRefreshBundleListClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("SCustomAssetManagerEditorWindow: OnRefreshBundleListClicked - Refresh button clicked"));
    
    // Refresh the bundle list
    RefreshBundleList();
    
    return FReply::Handled();
}

void SCustomAssetManagerEditorWindow::UpdateDependencyList()
{
    DependencyEntries.Empty();
    
    // Get the selected asset
    UCustomAssetBase* SelectedAsset = GetSelectedAsset();
    if (!SelectedAsset)
    {
        return;
    }
    
    // Add entries based on dependencies or dependents
    if (bShowingDependents)
    {
        // Show assets that depend on the selected asset
        for (const FCustomAssetDependency& Dependency : SelectedAsset->DependentAssets)
        {
            TSharedPtr<FDependencyEntry> Entry = MakeShared<FDependencyEntry>();
            Entry->AssetId = Dependency.DependentAssetId;
            Entry->DisplayName = FText::FromName(Dependency.DependentAssetId);
            Entry->bIsHardDependency = Dependency.bHardDependency;
            Entry->DependencyType = Dependency.DependencyType;
            
            DependencyEntries.Add(Entry);
        }
    }
    else
    {
        // Show assets that the selected asset depends on
        for (const FCustomAssetDependency& Dependency : SelectedAsset->Dependencies)
        {
            TSharedPtr<FDependencyEntry> Entry = MakeShared<FDependencyEntry>();
            Entry->AssetId = Dependency.DependentAssetId;
            Entry->DisplayName = FText::FromName(Dependency.DependentAssetId);
            Entry->bIsHardDependency = Dependency.bHardDependency;
            Entry->DependencyType = Dependency.DependencyType;
            
            DependencyEntries.Add(Entry);
        }
    }
    
    // Refresh the list view if valid
    if (DependencyListView.IsValid())
    {
        DependencyListView->RequestListRefresh();
    }
}

void SCustomAssetManagerEditorWindow::OnAssetSelectionChanged(TSharedPtr<FAssetEntry> SelectedItem, ESelectInfo::Type SelectInfo)
{
    if (SelectedItem.IsValid())
    {
        SelectedAssetId = SelectedItem->AssetId;
        UE_LOG(LogTemp, Warning, TEXT("SCustomAssetManagerEditorWindow: Asset selected: %s"), *SelectedAssetId.ToString());
        
        // Update the dependency view when an asset is selected
        UpdateDependencyList();
    }
    else
    {
        SelectedAssetId = NAME_None;
        UE_LOG(LogTemp, Warning, TEXT("SCustomAssetManagerEditorWindow: No asset selected"));
    }
}

void SCustomAssetManagerEditorWindow::OnBundleSelectionChanged(TSharedPtr<FBundleEntry> SelectedItem, ESelectInfo::Type SelectInfo) {}

FReply SCustomAssetManagerEditorWindow::OnExportAssetsToCSVClicked()
{
    // TODO: Implement CSV export functionality
    UE_LOG(LogTemp, Warning, TEXT("SCustomAssetManagerEditorWindow: OnExportAssetsToCSVClicked - CSV export functionality not implemented"));
    return FReply::Handled();
}

FReply SCustomAssetManagerEditorWindow::OnExportMemoryUsageToCSVClicked()
{
    // TODO: Implement CSV export functionality
    UE_LOG(LogTemp, Warning, TEXT("SCustomAssetManagerEditorWindow: OnExportMemoryUsageToCSVClicked - CSV export functionality not implemented"));
    return FReply::Handled();
}

FReply SCustomAssetManagerEditorWindow::OnExportDependencyGraphClicked()
{
    // TODO: Implement dependency graph export functionality
    UE_LOG(LogTemp, Warning, TEXT("SCustomAssetManagerEditorWindow: OnExportDependencyGraphClicked - Dependency graph export functionality not implemented"));
    return FReply::Handled();
}

FReply SCustomAssetManagerEditorWindow::OnLoadAssetClicked()
{
    if (!SelectedAssetId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("SCustomAssetManagerEditorWindow: Loading asset: %s"), *SelectedAssetId.ToString());
        
        // Get the asset manager
        UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
        
        // Load the asset
        UCustomAssetBase* Asset = AssetManager.LoadAssetById(SelectedAssetId);
        
        if (Asset)
        {
            UE_LOG(LogTemp, Warning, TEXT("SCustomAssetManagerEditorWindow: Asset loaded successfully"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("SCustomAssetManagerEditorWindow: Failed to load asset"));
        }
        
        // Refresh the asset list to update the status
        RefreshAssetList();
    }
    
    return FReply::Handled();
}

FReply SCustomAssetManagerEditorWindow::OnUnloadAssetClicked()
{
    if (!SelectedAssetId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("SCustomAssetManagerEditorWindow: Unloading asset: %s"), *SelectedAssetId.ToString());
        
        // Get the asset manager
        UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
        
        // Unload the asset
        bool bSuccess = AssetManager.UnloadAssetById(SelectedAssetId);
        
        if (bSuccess)
        {
            UE_LOG(LogTemp, Warning, TEXT("SCustomAssetManagerEditorWindow: Asset unloaded successfully"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("SCustomAssetManagerEditorWindow: Failed to unload asset"));
        }
        
        // Refresh the asset list to update the status
        RefreshAssetList();
    }
    
    return FReply::Handled();
}

FReply SCustomAssetManagerEditorWindow::OnLoadBundleClicked()
{
    // TODO: Implement bundle loading functionality
    UE_LOG(LogTemp, Warning, TEXT("SCustomAssetManagerEditorWindow: OnLoadBundleClicked - Bundle loading functionality not implemented"));
    return FReply::Handled();
}

FReply SCustomAssetManagerEditorWindow::OnUnloadBundleClicked()
{
    // TODO: Implement bundle unloading functionality
    UE_LOG(LogTemp, Warning, TEXT("SCustomAssetManagerEditorWindow: OnUnloadBundleClicked - Bundle unloading functionality not implemented"));
    return FReply::Handled();
}

FReply SCustomAssetManagerEditorWindow::OnToggleDependencyViewClicked()
{
    // TODO: Implement dependency view toggling functionality
    UE_LOG(LogTemp, Warning, TEXT("SCustomAssetManagerEditorWindow: OnToggleDependencyViewClicked - Dependency view toggling functionality not implemented"));
    return FReply::Handled();
}

TSharedRef<ITableRow> SCustomAssetManagerEditorWindow::GenerateAssetRow(TSharedPtr<FAssetEntry> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    if (!Item.IsValid())
    {
        return SNew(STableRow<TSharedPtr<FAssetEntry>>, OwnerTable);
    }
    
    return SNew(STableRow<TSharedPtr<FAssetEntry>>, OwnerTable)
        .Padding(FMargin(5, 2))
        .Style(&FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row"))
        [
            SNew(SHorizontalBox)
            
            // Asset ID
            + SHorizontalBox::Slot()
            .FillWidth(0.2f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(FText::FromName(Item->AssetId))
                .ToolTipText(FText::FromName(Item->AssetId))
            ]
            
            // Asset Type
            + SHorizontalBox::Slot()
            .FillWidth(0.1f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(FText::FromString(Item->AssetType))
                .ColorAndOpacity(GetAssetTypeColor(Item->AssetType))
            ]
            
            // Display Name
            + SHorizontalBox::Slot()
            .FillWidth(0.2f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(Item->DisplayName)
            ]
            
            // Loaded Status
            + SHorizontalBox::Slot()
            .FillWidth(0.1f)
            .VAlign(VAlign_Center)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(0, 0, 5, 0)
                [
                    SNew(STextBlock)
                    .Text(Item->bIsLoaded ? FText::FromString("Yes") : FText::FromString("No"))
                    .ColorAndOpacity(Item->bIsLoaded ? FLinearColor::Green : FLinearColor::Red)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(SBox)
                    .Visibility(Item->bIsLoaded ? EVisibility::Visible : EVisibility::Collapsed)
                    .WidthOverride(24)
                    .HeightOverride(24)
                    [
                        SNew(SButton)
                        .ButtonStyle(FCoreStyle::Get(), "NoBorder")
                        .ToolTipText(LOCTEXT("UnloadAssetTooltip", "Unload this asset"))
                        .OnClicked_Lambda([this, Item]() 
                        {
                            this->SelectedAssetId = Item->AssetId;
                            return this->OnUnloadAssetClicked();
                        })
                        .Content()
                        [
                            SNew(STextBlock)
                            .Font(FAppStyle::Get().GetFontStyle("FontAwesome.10"))
                            .Text(FText::FromString(TEXT("\xf00d"))) // X icon
                            .ColorAndOpacity(FLinearColor::Red)
                        ]
                    ]
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(SBox)
                    .Visibility(Item->bIsLoaded ? EVisibility::Collapsed : EVisibility::Visible)
                    .WidthOverride(24)
                    .HeightOverride(24)
                    [
                        SNew(SButton)
                        .ButtonStyle(FCoreStyle::Get(), "NoBorder")
                        .ToolTipText(LOCTEXT("LoadAssetTooltip", "Load this asset"))
                        .OnClicked_Lambda([this, Item]() 
                        {
                            this->SelectedAssetId = Item->AssetId;
                            return this->OnLoadAssetClicked();
                        })
                        .Content()
                        [
                            SNew(STextBlock)
                            .Font(FAppStyle::Get().GetFontStyle("FontAwesome.10"))
                            .Text(FText::FromString(TEXT("\xf055"))) // Plus circle icon
                            .ColorAndOpacity(FLinearColor::Green)
                        ]
                    ]
                ]
            ]
            
            // Memory Usage
            + SHorizontalBox::Slot()
            .FillWidth(0.1f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(FormatMemorySize(Item->MemoryUsage))
            ]
            
            // Version
            + SHorizontalBox::Slot()
            .FillWidth(0.1f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(FText::AsNumber(Item->Version))
            ]
            
            // Bundle Membership
            + SHorizontalBox::Slot()
            .FillWidth(0.15f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(FText::FromString(Item->GetBundleListString()))
                .ToolTipText(FText::FromString(Item->GetBundleListString()))
            ]
        ];
}

TSharedRef<ITableRow> SCustomAssetManagerEditorWindow::GenerateBundleRow(TSharedPtr<FBundleEntry> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    if (!Item.IsValid())
    {
        return SNew(STableRow<TSharedPtr<FBundleEntry>>, OwnerTable);
    }
    
    // Calculate the total memory for all assets in this bundle
    int64 TotalMemory = 0;
    int32 LoadedAssetCount = 0;
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    
    // Check all asset IDs in the bundle
    for (const FName& AssetId : Item->Bundle->AssetIds)
    {
        UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
        if (Asset)
        {
            TotalMemory += AssetManager.EstimateAssetMemoryUsage(Asset);
            LoadedAssetCount++;
        }
    }
    
    // For display, format memory size and create status text
    FText MemorySizeText = FormatMemorySize(TotalMemory);
    FText StatusText = FText::Format(LOCTEXT("BundleAssetStatus", "{0}/{1} assets loaded"), 
        FText::AsNumber(LoadedAssetCount), 
        FText::AsNumber(Item->Bundle->AssetIds.Num()));
    
    return SNew(STableRow<TSharedPtr<FBundleEntry>>, OwnerTable)
    [
        SNew(SHorizontalBox)
        
        // Bundle ID - Match the HeaderRow FillWidth (0.2f)
        + SHorizontalBox::Slot()
        .FillWidth(0.2f)
        .Padding(4, 0)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(FText::FromString(Item->Bundle->BundleId.ToString()))
        ]
        
        // Display Name - Match the HeaderRow FillWidth (0.2f)
        + SHorizontalBox::Slot()
        .FillWidth(0.2f)
        .Padding(4, 0)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(Item->Bundle->DisplayName)
        ]
        
        // Assets - Match the HeaderRow FillWidth (0.2f)
        + SHorizontalBox::Slot()
        .FillWidth(0.2f)
        .Padding(4, 0)
        .VAlign(VAlign_Center)
        [
            SNew(SHorizontalBox)
            +SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(STextBlock)
                .Text(FText::Format(LOCTEXT("AssetCountFormat", "{0} assets"), FText::AsNumber(Item->Bundle->AssetIds.Num())))
            ]
            +SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(FMargin(5, 0))
            [
                SNew(SButton)
                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                .Text(LOCTEXT("ViewAssetsButton", "View"))
                .ToolTipText(LOCTEXT("ViewAssetsTooltip", "View the assets in this bundle"))
                .IsEnabled(Item->Bundle->AssetIds.Num() > 0)
                .OnClicked_Lambda([this, Item]() {
                    // Show list of assets in this bundle
                    TSharedRef<SWindow> DialogWindow = SNew(SWindow)
                        .Title(FText::Format(LOCTEXT("AssetsInBundle", "Assets in {0}"), Item->Bundle->DisplayName))
                        .ClientSize(FVector2D(500, 400))
                        .SupportsMaximize(true)
                        .SupportsMinimize(false);
                    
                    // Create a list of assets
                    TArray<TSharedPtr<FAssetListItem>> AssetItems;
                    
                    // Fill the list with bundle assets from both Assets and AssetIds arrays
                    // First add assets from the Assets array
                    for (UCustomAssetBase* Asset : Item->Bundle->Assets)
                    {
                        if (Asset)
                        {
                            TSharedPtr<FAssetListItem> AssetItem = MakeShared<FAssetListItem>();
                            AssetItem->AssetId = Asset->AssetId;
                            AssetItem->DisplayName = Asset->DisplayName;
                            AssetItems.Add(AssetItem);
                        }
                    }
                    
                    // Then add any asset IDs that weren't already added
                    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
                    for (const FName& AssetId : Item->Bundle->AssetIds)
                    {
                        // Check if already added
                        bool bAlreadyAdded = false;
                        for (const TSharedPtr<FAssetListItem>& ExistingItem : AssetItems)
                        {
                            if (ExistingItem->AssetId == AssetId)
                            {
                                bAlreadyAdded = true;
                                break;
                            }
                        }
                        
                        if (!bAlreadyAdded)
                        {
                            TSharedPtr<FAssetListItem> AssetItem = MakeShared<FAssetListItem>();
                            AssetItem->AssetId = AssetId;
                            
                            // Try to get display name from loaded asset
                            UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
                            if (Asset)
                            {
                                AssetItem->DisplayName = Asset->DisplayName;
                            }
                            else
                            {
                                // Use ID as display name for unloaded assets
                                AssetItem->DisplayName = FText::FromName(AssetId);
                            }
                            
                            AssetItems.Add(AssetItem);
                        }
                    }
                    
                    // Dialog content continued...
                    
                    // Create a list view for the assets
                    TSharedPtr<SListView<TSharedPtr<FAssetListItem>>> AssetListView;
                    
                    DialogWindow->SetContent(
                        SNew(SVerticalBox)
                        +SVerticalBox::Slot()
                        .Padding(10)
                        .FillHeight(1.0f)
                        [
                            SAssignNew(AssetListView, SListView<TSharedPtr<FAssetListItem>>)
                            .ListItemsSource(&AssetItems)
                            .OnGenerateRow_Lambda([](TSharedPtr<FAssetListItem> Item, const TSharedRef<STableViewBase>& OwnerTable) {
                                return SNew(STableRow<TSharedPtr<FAssetListItem>>, OwnerTable)
                                [
                                    SNew(SHorizontalBox)
                                    +SHorizontalBox::Slot()
                                    .Padding(FMargin(5, 0))
                                    .AutoWidth()
                                    [
                                        SNew(STextBlock)
                                        .Text(FText::FromName(Item->AssetId))
                                        .MinDesiredWidth(150)
                                    ]
                                    +SHorizontalBox::Slot()
                                    .Padding(FMargin(5, 0))
                                    .AutoWidth()
                                    [
                                        SNew(STextBlock)
                                        .Text(Item->DisplayName)
                                        .MinDesiredWidth(200)
                                    ]
                                ];
                            })
                        ]
                        +SVerticalBox::Slot()
                        .Padding(10)
                        .AutoHeight()
                        .HAlign(HAlign_Right)
                        [
                            SNew(SButton)
                            .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                            .Text(LOCTEXT("CloseButton", "Close"))
                            .OnClicked_Lambda([DialogWindow]() {
                                FSlateApplication::Get().RequestDestroyWindow(DialogWindow);
                                return FReply::Handled();
                            })
                        ]
                    );
                    
                    FSlateApplication::Get().AddModalWindow(DialogWindow, FGlobalTabmanager::Get()->GetRootWindow());
                    
                    return FReply::Handled();
                })
            ]
        ]
        
        // Status - Match the HeaderRow FillWidth (0.15f)
        + SHorizontalBox::Slot()
        .FillWidth(0.15f)
        .Padding(4, 0)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(StatusText)
        ]
        
        // Memory - Match the HeaderRow FillWidth (0.1f)
        + SHorizontalBox::Slot()
        .FillWidth(0.1f)
        .Padding(4, 0)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(MemorySizeText)
        ]
        
        // Actions - Match the HeaderRow FillWidth (0.15f)
        + SHorizontalBox::Slot()
        .FillWidth(0.15f)
        .Padding(4, 0)
        .VAlign(VAlign_Center)
        [
            SNew(SHorizontalBox)
            
            // Load/Unload Button
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2, 0)
            [
                SNew(SButton)
                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                .Text(Item->Bundle->bIsLoaded ? LOCTEXT("UnloadButton", "Unload") : LOCTEXT("LoadButton", "Load"))
                .ToolTipText(Item->Bundle->bIsLoaded ? 
                    LOCTEXT("UnloadBundleTooltip", "Unload all assets in this bundle") : 
                    LOCTEXT("LoadBundleTooltip", "Load all assets in this bundle"))
                .OnClicked_Lambda([this, Item]() {
                    this->SelectedBundleId = Item->Bundle->BundleId;
                    if (Item->Bundle->bIsLoaded)
                    {
                        UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
                        AssetManager.UnloadBundle(Item->Bundle->BundleId);
                        RefreshBundleList();
                    }
                    else
                    {
                        UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
                        AssetManager.LoadBundle(Item->Bundle->BundleId);
                        RefreshBundleList();
                    }
                    return FReply::Handled();
                })
            ]
            
            // Remove Assets Button
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2, 0)
            [
                SNew(SButton)
                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                .Text(LOCTEXT("RemoveAssetsButton", "Remove"))
                .ToolTipText(LOCTEXT("RemoveAssetsTooltip", "Remove assets from this bundle"))
                .IsEnabled(Item->Bundle->AssetIds.Num() > 0)
                .OnClicked_Lambda([this, Item]() {
                    this->SelectedBundleId = Item->Bundle->BundleId;
                    this->ShowRemoveAssetsFromSelectedBundleDialog();
                    return FReply::Handled();
                })
            ]
            
            // Delete button
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2, 0)
            [
                SNew(SButton)
                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                .Text(LOCTEXT("DeleteButton", "Delete"))
                .ToolTipText(LOCTEXT("DeleteTooltip", "Delete this bundle"))
                .OnClicked_Lambda([this, Item]() {
                    ShowDeleteBundleDialog(Item->Bundle);
                    return FReply::Handled();
                })
            ]
        ]
    ];
}

TSharedRef<ITableRow> SCustomAssetManagerEditorWindow::GenerateDependencyRow(TSharedPtr<FDependencyEntry> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    if (!Item.IsValid())
    {
        return SNew(STableRow<TSharedPtr<FDependencyEntry>>, OwnerTable);
    }
    
    return SNew(STableRow<TSharedPtr<FDependencyEntry>>, OwnerTable)
    [
        SNew(SHorizontalBox)
        
        // Asset ID
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(4, 0)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(FText::FromString(Item->AssetId.ToString()))
            .MinDesiredWidth(120)
        ]
        
        // Name
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(4, 0)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(Item->DisplayName)
            .MinDesiredWidth(150)
        ]
        
        // Dependency Type
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(4, 0)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(FText::FromString(Item->DependencyType.ToString()))
            .MinDesiredWidth(100)
        ]
        
        // Hard/Soft Dependency
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(4, 0)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(Item->bIsHardDependency ? LOCTEXT("HardDependency", "Hard") : LOCTEXT("SoftDependency", "Soft"))
            .ColorAndOpacity(Item->bIsHardDependency ? FLinearColor(1.0f, 0.4f, 0.4f) : FLinearColor(0.4f, 0.4f, 1.0f))
            .MinDesiredWidth(80)
        ]
    ];
}

FText SCustomAssetManagerEditorWindow::FormatMemorySize(int64 MemoryInBytes) const
{
    if (MemoryInBytes < 1024)
    {
        return FText::Format(LOCTEXT("MemorySizeBytes", "{0} B"), FText::AsNumber(MemoryInBytes));
    }
    else if (MemoryInBytes < 1024 * 1024)
    {
        float KBytes = MemoryInBytes / 1024.0f;
        FNumberFormattingOptions Options;
        Options.MinimumFractionalDigits = 0;
        Options.MaximumFractionalDigits = 2;
        return FText::Format(LOCTEXT("MemorySizeKilobytes", "{0} KB"), FText::AsNumber(KBytes, &Options));
    }
    else if (MemoryInBytes < 1024 * 1024 * 1024)
    {
        float MBytes = MemoryInBytes / (1024.0f * 1024.0f);
        FNumberFormattingOptions Options;
        Options.MinimumFractionalDigits = 0;
        Options.MaximumFractionalDigits = 2;
        return FText::Format(LOCTEXT("MemorySizeMegabytes", "{0} MB"), FText::AsNumber(MBytes, &Options));
    }
    else
    {
        float GBytes = MemoryInBytes / (1024.0f * 1024.0f * 1024.0f);
        FNumberFormattingOptions Options;
        Options.MinimumFractionalDigits = 0;
        Options.MaximumFractionalDigits = 2;
        return FText::Format(LOCTEXT("MemorySizeGigabytes", "{0} GB"), FText::AsNumber(GBytes, &Options));
    }
}

bool SCustomAssetManagerEditorWindow::IsAssetSelected() const
{
    return AssetListView->GetNumItemsSelected() > 0;
}

bool SCustomAssetManagerEditorWindow::IsBundleSelected() const
{
    return BundleListView->GetNumItemsSelected() > 0;
}

FText SCustomAssetManagerEditorWindow::GetCurrentMemoryUsageText() const
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    // Replace with actual method call if available, otherwise use a placeholder
    return FormatMemorySize(0); // Replace with AssetManager.GetCurrentMemoryUsage() if available
}

FText SCustomAssetManagerEditorWindow::GetMemoryThresholdText() const
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    // Replace with actual method call if available, otherwise use a placeholder
    return FormatMemorySize(0); // Replace with AssetManager.GetMemoryThreshold() if available
}

FText SCustomAssetManagerEditorWindow::GetMemoryPolicyText() const
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    // Replace with actual enum if available
    int32 MemoryPolicy = 0; // Replace with AssetManager.GetMemoryPolicy() if available
    
    switch (MemoryPolicy)
    {
        case 0: // EMemoryManagementPolicy::KeepAll
            return LOCTEXT("KeepAllPolicy", "Keep All");
        case 1: // EMemoryManagementPolicy::UnloadLRU
            return LOCTEXT("UnloadLRUPolicy", "Unload Least Recently Used");
        case 2: // EMemoryManagementPolicy::UnloadLFU
            return LOCTEXT("UnloadLFUPolicy", "Unload Least Frequently Used");
        case 3: // EMemoryManagementPolicy::Custom
            return LOCTEXT("CustomPolicy", "Custom");
        default:
            return LOCTEXT("UnknownPolicy", "Unknown");
    }
}

FText SCustomAssetManagerEditorWindow::GetDependencyHeaderText() const
{
    TArray<TSharedPtr<FAssetEntry>> SelectedAssets = AssetListView->GetSelectedItems();
    if (SelectedAssets.Num() > 0)
    {
        return FText::Format(LOCTEXT("AssetDependenciesTitle", "Dependencies for {0}"), 
            FText::FromName(SelectedAssets[0]->AssetId));
    }
    return LOCTEXT("NoDependenciesTitle", "Asset Dependencies");
}

FReply SCustomAssetManagerEditorWindow::OnCreateBundleClicked()
{
    // Create a dialog to get bundle name
    TSharedRef<SEditableTextBox> BundleNameTextBox = SNew(SEditableTextBox)
        .HintText(LOCTEXT("BundleNameHint", "Enter bundle name"))
        .MinDesiredWidth(250);
        
    // Show dialog - use TSharedPtr instead of TSharedRef to better handle lifetime
    TSharedPtr<SWindow> DialogWindow = SNew(SWindow)
        .Title(LOCTEXT("CreateBundleTitle", "Create New Asset Bundle"))
        .ClientSize(FVector2D(400, 100))
        .SupportsMaximize(false)
        .SupportsMinimize(false);
        
    DialogWindow->SetContent(
        SNew(SVerticalBox)
        +SVerticalBox::Slot()
        .Padding(10)
        .AutoHeight()
        [
            BundleNameTextBox
        ]
        +SVerticalBox::Slot()
        .Padding(10)
        .AutoHeight()
        .HAlign(HAlign_Right)
        [
            SNew(SHorizontalBox)
            +SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("CreateButton", "Create"))
                .OnClicked_Lambda([this, BundleNameTextBox, WeakDialogWindow = TWeakPtr<SWindow>(DialogWindow)]()
                {
                    FString BundleName = BundleNameTextBox->GetText().ToString();
                    if (BundleName.IsEmpty())
                    {
                        // Use a default name if none provided
                        BundleName = FString::Printf(TEXT("Bundle_%s"), *FGuid::NewGuid().ToString());
                    }
                    
                    UE_LOG(LogTemp, Log, TEXT("Creating new bundle with name: %s"), *BundleName);
                    
                    // Create the bundle with a valid outer
                    UCustomAssetBundle* NewBundle = NewObject<UCustomAssetBundle>(GetTransientPackage());
                    if (!NewBundle)
                    {
                        UE_LOG(LogTemp, Error, TEXT("Failed to create new bundle object"));
                        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("FailedToCreateBundle", "Failed to create new bundle."));
                    }
                    else
                    {
                        // Set the display name first
                        NewBundle->DisplayName = FText::FromString(BundleName);
                        UE_LOG(LogTemp, Log, TEXT("Set display name to: %s"), *NewBundle->DisplayName.ToString());
                        
                        // Always generate a new GUID to ensure uniqueness
                        FGuid BundleGuid = FGuid::NewGuid();
                        NewBundle->BundleId = FName(*BundleGuid.ToString());
                        
                        // Validate the bundle ID
                        if (NewBundle->BundleId.IsNone())
                        {
                            UE_LOG(LogTemp, Warning, TEXT("Generated bundle ID is None, generating another ID"));
                            BundleGuid = FGuid::NewGuid();
                            NewBundle->BundleId = FName(*BundleGuid.ToString());
                        }
                        
                        UE_LOG(LogTemp, Log, TEXT("Created new bundle with ID: %s, DisplayName: %s"), 
                            *NewBundle->BundleId.ToString(), 
                            *NewBundle->DisplayName.ToString());
                        
                        // Add to manager and refresh
                        UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
                        AssetManager.RegisterBundle(NewBundle);
                        
                        try
                        {
                            // Save the bundle to the project
                            UE_LOG(LogTemp, Log, TEXT("Saving new bundle to project: ID=%s, DisplayName=%s"), 
                                *NewBundle->BundleId.ToString(), 
                                *NewBundle->DisplayName.ToString());
                                
                            bool bSaved = AssetManager.SaveBundle(NewBundle, TEXT("/Game/Bundles"));
                            if (!bSaved)
                            {
                                UE_LOG(LogTemp, Warning, TEXT("Failed to save bundle %s"), *NewBundle->BundleId.ToString());
                            }
                            else
                            {
                                // Verify the bundle was saved with the correct properties
                                UCustomAssetBundle* SavedBundle = AssetManager.GetBundleById(NewBundle->BundleId);
                                if (SavedBundle)
                                {
                                    UE_LOG(LogTemp, Log, TEXT("Saved bundle verified: ID=%s, DisplayName=%s"), 
                                        *SavedBundle->BundleId.ToString(), 
                                        *SavedBundle->DisplayName.ToString());
                                }
                            }
                        }
                        catch (const std::exception& e)
                        {
                            UE_LOG(LogTemp, Error, TEXT("Exception when saving new bundle: %s"), UTF8_TO_TCHAR(e.what()));
                        }
                    }
                    
                    // First close dialog safely
                    TSharedPtr<SWindow> PinnedWindow = WeakDialogWindow.Pin();
                    if (PinnedWindow.IsValid())
                    {
                        FSlateApplication::Get().RequestDestroyWindow(PinnedWindow.ToSharedRef());
                    }
                    
                    // Then refresh the UI after window is closed
                    RefreshBundleList();
                    
                    return FReply::Handled();
                })
            ]
            +SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("CancelButton", "Cancel"))
                .OnClicked_Lambda([WeakDialogWindow = TWeakPtr<SWindow>(DialogWindow)]()
                {
                    TSharedPtr<SWindow> PinnedWindow = WeakDialogWindow.Pin();
                    if (PinnedWindow.IsValid())
                    {
                        FSlateApplication::Get().RequestDestroyWindow(PinnedWindow.ToSharedRef());
                    }
                    return FReply::Handled();
                })
            ]
        ]
    );
    
    FSlateApplication::Get().AddModalWindow(DialogWindow.ToSharedRef(), FGlobalTabmanager::Get()->GetRootWindow());
    
    return FReply::Handled();
}

UCustomAssetBase* SCustomAssetManagerEditorWindow::GetSelectedAsset() const
{
    if (SelectedAssetId.IsNone())
    {
        return nullptr;
    }
    
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    return AssetManager.GetAssetById(SelectedAssetId);
}

FReply SCustomAssetManagerEditorWindow::OnAddAssetToBundleClicked()
{
    if (!IsAssetSelected())
    {
        return FReply::Handled();
    }
    
    // Get all selected assets
    TArray<TSharedPtr<FAssetEntry>> SelectedAssets = AssetListView->GetSelectedItems();
    if (SelectedAssets.Num() == 0)
    {
        return FReply::Handled();
    }
    
    // Collect all selected asset IDs
    TArray<FName> SelectedAssetIds;
    for (const TSharedPtr<FAssetEntry>& AssetEntry : SelectedAssets)
    {
        // Add the asset ID to our list of selected assets, only if it's valid
        if (!AssetEntry->AssetId.IsNone())
        {
            SelectedAssetIds.Add(AssetEntry->AssetId);
            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] OnAddAssetToBundleClicked: Selected asset for adding: %s"), 
                *AssetEntry->AssetId.ToString());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] OnAddAssetToBundleClicked: Skipping None AssetId in selected assets"));
        }
    }
    
    if (SelectedAssetIds.Num() == 0)
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoValidAssetsSelected", "No valid assets selected. Please select at least one valid asset."));
        return FReply::Handled();
    }
    
    // Get all available bundles
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    TArray<UCustomAssetBundle*> AllBundles;
    AssetManager.GetAllBundles(AllBundles);
    
    if (AllBundles.Num() == 0)
    {
        // No bundles exist yet, show a message and allow user to create one
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoBundlesExist", "No bundles exist yet. Create a bundle first."));
        return FReply::Handled();
    }
    
    // Create a list of bundles
    TArray<TSharedPtr<FBundleEntry>> BundleOptions;
    for (UCustomAssetBundle* Bundle : AllBundles)
    {
        if (!Bundle)
        {
            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] OnAddAssetToBundleClicked: Skipping null bundle"));
            continue;
        }
        
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] OnAddAssetToBundleClicked: Processing bundle: ID=%s, DisplayName=%s"), 
            *Bundle->BundleId.ToString(), 
            *Bundle->DisplayName.ToString());
        
        // Check if any of the selected assets are already in this bundle
        bool bAssetAlreadyInBundle = false;
        for (const FName& AssetId : SelectedAssetIds)
        {
            if (Bundle->ContainsAsset(AssetId))
            {
                bAssetAlreadyInBundle = true;
                UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] OnAddAssetToBundleClicked: Asset %s is already in bundle %s, skipping from options"),
                    *AssetId.ToString(), *Bundle->BundleId.ToString());
                break;
            }
        }
        
        // Only add the bundle to options if none of the selected assets are already in it
        if (!bAssetAlreadyInBundle)
        {
            TSharedPtr<FBundleEntry> Entry = MakeShared<FBundleEntry>();
            Entry->Bundle = Bundle;
            BundleOptions.Add(Entry);
        }
    }
    
    // Check if we have any bundles available after filtering
    if (BundleOptions.Num() == 0)
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoAvailableBundles", 
            "All selected assets are already in all existing bundles. Create a new bundle first."));
        return FReply::Handled();
    }
    
    // Create a dialog to select a bundle
    TSharedRef<SWindow> DialogWindow = SNew(SWindow)
        .Title(LOCTEXT("SelectBundle", "Select Bundle"))
        .ClientSize(FVector2D(400, 300))
        .SupportsMaximize(false)
        .SupportsMinimize(false);
        
    // Track the selected bundle
    TSharedPtr<UCustomAssetBundle*> SelectedBundle = MakeShared<UCustomAssetBundle*>(nullptr);
    
    // Create a list view for the bundles
    TSharedPtr<SListView<TSharedPtr<FBundleEntry>>> BundleSelectionListView;
    
    DialogWindow->SetContent(
        SNew(SVerticalBox)
        +SVerticalBox::Slot()
        .FillHeight(1.0f)
        .Padding(10)
        [
            SAssignNew(BundleSelectionListView, SListView<TSharedPtr<FBundleEntry>>)
            .ListItemsSource(&BundleOptions)
            .OnGenerateRow_Lambda([](TSharedPtr<FBundleEntry> Item, const TSharedRef<STableViewBase>& OwnerTable) {
                return SNew(STableRow<TSharedPtr<FBundleEntry>>, OwnerTable)
                [
                    SNew(STextBlock)
                    .Text(Item->Bundle->DisplayName)
                ];
            })
            .OnSelectionChanged_Lambda([SelectedBundle](TSharedPtr<FBundleEntry> Item, ESelectInfo::Type SelectType) {
                if (Item.IsValid())
                {
                    *SelectedBundle = Item->Bundle;
                }
                else
                {
                    *SelectedBundle = nullptr;
                }
            })
        ]
        +SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        .HAlign(HAlign_Right)
        [
            SNew(SHorizontalBox)
            +SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("AddButton", "Add"))
                .OnClicked_Lambda([this, DialogWindow, SelectedBundle, SelectedAssetIds]() {
                    if (!SelectedBundle)
                    {
                        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoBundleSelected", "No bundle selected. Please select a bundle first."));
                        return FReply::Handled();
                    }
                
                    if (*SelectedBundle == nullptr)
                    {
                        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoBundleSelected", "No bundle selected. Please select a bundle first."));
                        return FReply::Handled();
                    }
                    
                    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] OnAddAssetToBundleClicked: Adding assets to bundle: ID=%s, DisplayName=%s"), 
                        *(*SelectedBundle)->BundleId.ToString(), 
                        *(*SelectedBundle)->DisplayName.ToString());
                    
                    // CRITICAL: Verify the bundle is in a good state before adding assets
                    (*SelectedBundle)->DebugPrintContents(TEXT("BEFORE_ADDING_ASSETS"));
                    
                    if ((*SelectedBundle)->BundleId.IsNone())
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] OnAddAssetToBundleClicked: Adding assets to bundle with None ID, generating a new ID"));
                        // Generate a new ID for the bundle
                        FGuid NewGuid = FGuid::NewGuid();
                        (*SelectedBundle)->BundleId = FName(*NewGuid.ToString());
                    }
                    
                    // Store the bundle's display name before operations
                    FText OriginalBundleName = (*SelectedBundle)->DisplayName;
                    
                    // Add all selected assets to the bundle
                    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
                    int32 AddedCount = 0;
                    
                    // CRITICAL FIX: Make sure we're only adding selected assets 
                    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] OnAddAssetToBundleClicked: About to add %d selected assets to bundle"), 
                        SelectedAssetIds.Num());
                    
                    for (const FName& AssetId : SelectedAssetIds)
                    {
                        if (AssetId.IsNone())
                        {
                            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] OnAddAssetToBundleClicked: Skipping None AssetId"));
                            continue; // Skip invalid assets
                        }
                        
                        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] OnAddAssetToBundleClicked: Processing asset %s for bundle %s"), 
                            *AssetId.ToString(), *(*SelectedBundle)->BundleId.ToString());
                            
                        // Add the asset to the bundle - the AddAsset method handles both loaded and unloaded assets
                        if (!(*SelectedBundle)->ContainsAsset(AssetId))
                        {
                            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] OnAddAssetToBundleClicked: Asset %s not in bundle, adding it"), 
                                *AssetId.ToString());
                                
                            (*SelectedBundle)->AddAsset(AssetId);
                            
                            // Make sure the asset pointer is also added if it's loaded
                            UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
                            if (Asset && !(*SelectedBundle)->Assets.Contains(Asset))
                            {
                                UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] OnAddAssetToBundleClicked: Adding loaded asset to Assets array"));
                                (*SelectedBundle)->Assets.Add(Asset);
                            }
                            
                            AddedCount++;
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] OnAddAssetToBundleClicked: Asset %s already in bundle, skipping"), 
                                *AssetId.ToString());
                        }
                    }
                    
                    // Show a message about how many assets were added
                    FText Message = FText::Format(LOCTEXT("AssetsAddedToBundle", "{0} assets added to bundle {1}"), 
                        FText::AsNumber(AddedCount), 
                        OriginalBundleName);
                    
                    // Save the bundle and refresh the UI
                    if (AddedCount > 0)
                    {
                        try
                        {
                            // Save the bundle if any assets were added
                            // Get a local reference to the bundle before saving
                            UCustomAssetBundle* BundleToSave = *SelectedBundle;
                            FName BundleId = BundleToSave->BundleId;
                            
                            // CRITICAL DIAGNOSTICS: Print bundle contents before saving
                            BundleToSave->DebugPrintContents(TEXT("BEFORE_SAVE"));
                            
                            // CRITICAL FAILSAFE: Double check if assets are missing
                            if (BundleToSave->AssetIds.Num() == 0 && AddedCount > 0)
                            {
                                UE_LOG(LogTemp, Error, TEXT("[CRITICAL] SaveBundle: CRITICAL ISSUE - Bundle reports %d assets added but AssetIds array is empty!"), AddedCount);
                                
                                // Emergency recovery - manually add assets that we know should be in the bundle
                                for (const FName& AssetId : SelectedAssetIds)
                                {
                                    UE_LOG(LogTemp, Error, TEXT("[CRITICAL] SaveBundle: EMERGENCY - Manually adding asset %s to bundle"), *AssetId.ToString());
                                    BundleToSave->AssetIds.AddUnique(AssetId);
                                    
                                    // Also add to loaded assets if possible
                                    UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
                                    if (Asset && !BundleToSave->Assets.Contains(Asset))
                                    {
                                        BundleToSave->Assets.Add(Asset);
                                    }
                                }
                                
                                // Print bundle contents after emergency recovery
                                BundleToSave->DebugPrintContents(TEXT("EMERGENCY_RECOVERY"));
                            }
                            
                            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] Saving bundle after adding assets: ID=%s, DisplayName=%s, Assets=%d"), 
                                *BundleId.ToString(), 
                                *BundleToSave->DisplayName.ToString(),
                                BundleToSave->AssetIds.Num());
                            
                            // Use AssetManager.SaveBundle directly instead of Bundle->Save()
                            bool bSaved = AssetManager.SaveBundle(BundleToSave, TEXT("/Game/Bundles"));
                            
                            if (!bSaved)
                            {
                                UE_LOG(LogTemp, Error, TEXT("Failed to save bundle %s"), *BundleId.ToString());
                                FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("FailedToSaveBundle", 
                                    "Failed to save bundle {0}. Check the logs for more information."), OriginalBundleName));
                            }
                            
                            // Verify that assets were saved in the bundle
                            UCustomAssetBundle* SavedBundle = AssetManager.GetBundleById(BundleId);
                            if (SavedBundle)
                            {
                                // CRITICAL DIAGNOSTICS: Print saved bundle contents
                                SavedBundle->DebugPrintContents(TEXT("AFTER_SAVE"));
                                
                                UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] After save: Bundle %s has %d assets"), 
                                    *SavedBundle->BundleId.ToString(), 
                                    SavedBundle->AssetIds.Num());
                                    
                                // CRITICAL FAILSAFE: If bundle still has no assets after saving, try one more time
                                if (SavedBundle->AssetIds.Num() == 0 && AddedCount > 0)
                                {
                                    UE_LOG(LogTemp, Error, TEXT("[CRITICAL] FINAL ATTEMPT: Bundle still has no assets after save! Trying one last emergency save"));
                                    
                                    // Emergency recovery - manually add assets again to the saved bundle
                                    for (const FName& AssetId : SelectedAssetIds)
                                    {
                                        UE_LOG(LogTemp, Error, TEXT("[CRITICAL] FINAL ATTEMPT: Manually adding asset %s to saved bundle"), *AssetId.ToString());
                                        SavedBundle->AssetIds.AddUnique(AssetId);
                                        
                                        // Also add to loaded assets if possible
                                        UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
                                        if (Asset && !SavedBundle->Assets.Contains(Asset))
                                        {
                                            SavedBundle->Assets.Add(Asset);
                                        }
                                    }
                                    
                                    // Try saving one more time
                                    SavedBundle->DebugPrintContents(TEXT("FINAL_ATTEMPT"));
                                    AssetManager.SaveBundle(SavedBundle, TEXT("/Game/Bundles"));
                                }
                            }
                            
                            // Refresh the asset list to update bundle membership
                            RefreshAssetList();
                            
                            // Refresh the bundle list to show updated asset count
                            RefreshBundleList();
                        }
                        catch (const std::exception& e)
                        {
                            UE_LOG(LogTemp, Error, TEXT("Exception when saving bundle: %s"), UTF8_TO_TCHAR(e.what()));
                            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("Error saving bundle: %s"), UTF8_TO_TCHAR(e.what()))));
                        }
                    }
                    
                    FMessageDialog::Open(EAppMsgType::Ok, Message);
                    
                    // Close the dialog
                    FSlateApplication::Get().RequestDestroyWindow(DialogWindow);
                    return FReply::Handled();
                })
            ]
            +SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("CancelButton", "Cancel"))
                .OnClicked_Lambda([DialogWindow]() {
                    FSlateApplication::Get().RequestDestroyWindow(DialogWindow);
                    return FReply::Handled();
                })
            ]
        ]
    );
    
    FSlateApplication::Get().AddModalWindow(DialogWindow, FGlobalTabmanager::Get()->GetRootWindow());
    
    return FReply::Handled();
}

void SCustomAssetManagerEditorWindow::ShowRemoveFromBundleDialog(const FName& AssetId)
{
    if (AssetId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] ShowRemoveFromBundleDialog called with None AssetId"));
        return;
    }

    // Get the asset manager
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    
    // Get the asset - can be null for unloaded assets
    UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
    
    // CRITICAL FIX: Get all bundles this asset is in
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] ShowRemoveFromBundleDialog: Checking bundles containing asset %s"), 
        *AssetId.ToString());
    
    // Get all bundles this asset is in
    TArray<UCustomAssetBundle*> AssetBundles;
    TArray<UCustomAssetBundle*> AllBundles;
    AssetManager.GetAllBundles(AllBundles);
    
    // Manually check each bundle to verify the asset is really in it
    for (UCustomAssetBundle* Bundle : AllBundles)
    {
        if (Bundle && !Bundle->BundleId.IsNone())
        {
            // Explicitly check if the bundle contains the asset
            bool bContainsAsset = Bundle->ContainsAsset(AssetId);
            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] ShowRemoveFromBundleDialog: Checking bundle %s (%s) - Contains asset: %s"), 
                *Bundle->BundleId.ToString(), *Bundle->DisplayName.ToString(), bContainsAsset ? TEXT("Yes") : TEXT("No"));
                
            if (bContainsAsset)
            {
                AssetBundles.Add(Bundle);
            }
        }
        else if (Bundle && Bundle->BundleId.IsNone())
        {
            UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] ShowRemoveFromBundleDialog: Skipping bundle with None ID"));
        }
    }
    
    if (AssetBundles.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] ShowRemoveFromBundleDialog: Asset %s is not in any bundles"), *AssetId.ToString());
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("AssetNotInAnyBundles", "This asset is not in any bundles."));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] ShowRemoveFromBundleDialog: Asset %s is in %d bundles"), 
        *AssetId.ToString(), AssetBundles.Num());
    
    // Create a dialog to select which bundle to remove from
    TSharedRef<SWindow> DialogWindow = SNew(SWindow)
        .Title(LOCTEXT("RemoveFromBundleTitle", "Remove from Bundle"))
        .ClientSize(FVector2D(400, 300))
        .SupportsMaximize(false)
        .SupportsMinimize(false);
        
    // Track the selected bundle
    TSharedPtr<UCustomAssetBundle*> SelectedBundle = MakeShared<UCustomAssetBundle*>(nullptr);
    
    // Create a list of bundles
    TArray<TSharedPtr<FBundleEntry>> BundleOptions;
    for (UCustomAssetBundle* Bundle : AssetBundles)
    {
        TSharedPtr<FBundleEntry> Entry = MakeShared<FBundleEntry>();
        Entry->Bundle = Bundle;
        BundleOptions.Add(Entry);
    }
    
    // Create a list view for the bundles
    TSharedPtr<SListView<TSharedPtr<FBundleEntry>>> BundleSelectionListView;
    
    // Get display name for the asset (either from the loaded asset or from the ID)
    FText AssetDisplayName = Asset ? Asset->DisplayName : FText::FromName(AssetId);
    
    DialogWindow->SetContent(
        SNew(SVerticalBox)
        +SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        [
            SNew(STextBlock)
            .Text(FText::Format(LOCTEXT("SelectBundleToRemoveFrom", "Select bundle to remove {0} from:"), AssetDisplayName))
        ]
        +SVerticalBox::Slot()
        .FillHeight(1.0f)
        .Padding(10)
        [
            SAssignNew(BundleSelectionListView, SListView<TSharedPtr<FBundleEntry>>)
            .ListItemsSource(&BundleOptions)
            .OnGenerateRow_Lambda([](TSharedPtr<FBundleEntry> Item, const TSharedRef<STableViewBase>& OwnerTable) {
                return SNew(STableRow<TSharedPtr<FBundleEntry>>, OwnerTable)
                [
                    SNew(STextBlock)
                    .Text(Item->Bundle->DisplayName)
                ];
            })
            .OnSelectionChanged_Lambda([SelectedBundle](TSharedPtr<FBundleEntry> Item, ESelectInfo::Type SelectType) {
                if (Item.IsValid())
                {
                    *SelectedBundle = Item->Bundle;
                }
                else
                {
                    *SelectedBundle = nullptr;
                }
            })
        ]
        +SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        .HAlign(HAlign_Right)
        [
            SNew(SHorizontalBox)
            +SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("RemoveButton", "Remove"))
                .OnClicked_Lambda([this, DialogWindow, SelectedBundle, AssetId, &AssetManager]() {
                    if (!SelectedBundle || !(*SelectedBundle))
                    {
                        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoBundleSelected", "No bundle selected for removal. Please select a bundle first."));
                        return FReply::Handled();
                    }
                    
                    UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] RemoveFromBundle: Removing asset %s from bundle %s"), 
                        *AssetId.ToString(), *(*SelectedBundle)->BundleId.ToString());
                    
                    // Double-check the bundle ID
                    if ((*SelectedBundle)->BundleId.IsNone())
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] RemoveFromBundle: Bundle has None ID, generating a new ID"));
                        // Generate a new ID for the bundle
                        FGuid NewGuid = FGuid::NewGuid();
                        (*SelectedBundle)->BundleId = FName(*NewGuid.ToString());
                    }
                    
                    // Verify the asset is actually in the bundle
                    if (!(*SelectedBundle)->ContainsAsset(AssetId))
                    {
                        UE_LOG(LogTemp, Error, TEXT("[CRITICAL] RemoveFromBundle: Asset %s is not in bundle %s!"), 
                            *AssetId.ToString(), *(*SelectedBundle)->BundleId.ToString());
                            
                        FMessageDialog::Open(EAppMsgType::Ok, 
                            FText::Format(LOCTEXT("AssetNotInBundle", "Asset '{0}' is not in the selected bundle."), 
                            FText::FromName(AssetId)));
                            
                        return FReply::Handled();
                    }
                    
                    // CRITICAL FIX: Explicitly log the bundle contents before removal
                    (*SelectedBundle)->DebugPrintContents(TEXT("BEFORE_ASSET_REMOVAL"));
                    
                    // Remove the asset from the bundle (both ID and reference)
                    (*SelectedBundle)->RemoveAsset(AssetId);
                    
                    // CRITICAL FIX: Also explicitly remove the asset reference if it's loaded
                    UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
                    if (Asset)
                    {
                        int32 RemovedCount = (*SelectedBundle)->Assets.Remove(Asset);
                        UE_LOG(LogTemp, Warning, TEXT("[CRITICAL] RemoveFromBundle: Explicitly removed %d instances of asset from Assets array"), 
                            RemovedCount);
                    }
                    
                    // Verify removal was successful
                    if ((*SelectedBundle)->ContainsAsset(AssetId))
                    {
                        UE_LOG(LogTemp, Error, TEXT("[CRITICAL] RemoveFromBundle: Failed to remove asset %s from bundle %s!"), 
                            *AssetId.ToString(), *(*SelectedBundle)->BundleId.ToString());
                            
                        FMessageDialog::Open(EAppMsgType::Ok, 
                            FText::Format(LOCTEXT("FailedToRemoveAsset", "Failed to remove asset '{0}' from the bundle. Check logs."), 
                            FText::FromName(AssetId)));
                            
                        return FReply::Handled();
                    }
                    
                    // Log the bundle contents after removal
                    (*SelectedBundle)->DebugPrintContents(TEXT("AFTER_ASSET_REMOVAL"));
                    
                    try
                    {
                        // Save the bundle
                        (*SelectedBundle)->Save();
                        
                        // Refresh the asset list to update bundle membership
                        RefreshAssetList();
                        
                        // Refresh the bundle list to show updated asset count
                        RefreshBundleList();
                        
                        FMessageDialog::Open(EAppMsgType::Ok, 
                            FText::Format(LOCTEXT("AssetRemovedFromBundle", "Asset '{0}' removed from bundle '{1}'"), 
                            FText::FromName(AssetId), 
                            (*SelectedBundle)->DisplayName));
                    }
                    catch (const std::exception& e)
                    {
                        UE_LOG(LogTemp, Error, TEXT("Exception when saving bundle after removal: %s"), UTF8_TO_TCHAR(e.what()));
                        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("Error saving bundle: %s"), UTF8_TO_TCHAR(e.what()))));
                    }
                    
                    // Close the dialog
                    FSlateApplication::Get().RequestDestroyWindow(DialogWindow);
                    return FReply::Handled();
                })
            ]
            +SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("CancelButton", "Cancel"))
                .OnClicked_Lambda([DialogWindow]() {
                    FSlateApplication::Get().RequestDestroyWindow(DialogWindow);
                    return FReply::Handled();
                })
            ]
        ]
    );
    
    FSlateApplication::Get().AddModalWindow(DialogWindow, FGlobalTabmanager::Get()->GetRootWindow());
}

FReply SCustomAssetManagerEditorWindow::OnRemoveAssetFromBundleClicked()
{
    // Get the selected asset
    TArray<TSharedPtr<FAssetEntry>> SelectedAssets = AssetListView->GetSelectedItems();
    if (SelectedAssets.Num() == 0)
    {
        return FReply::Handled();
    }
    
    // Show the dialog for the first selected asset
    ShowRemoveFromBundleDialog(SelectedAssets[0]->AssetId);
    
    return FReply::Handled();
}

FReply SCustomAssetManagerEditorWindow::OnImportAssetsFromCSVClicked()
{
    // Open a file dialog to select the CSV file
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (DesktopPlatform)
    {
        TArray<FString> OpenFilenames;
        bool bOpened = DesktopPlatform->OpenFileDialog(
            FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
            FText::FromString("Import Assets from CSV").ToString(),
            FPaths::ProjectDir(),
            TEXT(""),
            TEXT("CSV Files (*.csv)|*.csv"),
            EFileDialogFlags::None,
            OpenFilenames
        );

        if (bOpened && OpenFilenames.Num() > 0)
        {
            FString SelectedFile = OpenFilenames[0];
            if (ImportAssetsFromCSV(SelectedFile))
            {
                // If import was successful, refresh the asset list
                RefreshAssetList();
                return FReply::Handled();
            }
        }
    }
    
    return FReply::Handled();
}

bool SCustomAssetManagerEditorWindow::ImportAssetsFromCSV(const FString& FilePath)
{
    // Read the CSV file
    FString FileContent;
    if (!FFileHelper::LoadFileToString(FileContent, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load CSV file: %s"), *FilePath);
        return false;
    }
    
    // Split the content into lines
    TArray<FString> Lines;
    FileContent.ParseIntoArrayLines(Lines, false);
    
    if (Lines.Num() < 2) // Need at least header + one data row
    {
        UE_LOG(LogTemp, Error, TEXT("CSV file is empty or contains only headers: %s"), *FilePath);
        return false;
    }
    
    // Parse the header line to get column indices
    TArray<FString> Headers;
    Lines[0].ParseIntoArray(Headers, TEXT(","), true);
    
    // Find the indices of required columns
    int32 AssetIdIndex = Headers.Find(TEXT("AssetId"));
    int32 DisplayNameIndex = Headers.Find(TEXT("DisplayName"));
    int32 DescriptionIndex = Headers.Find(TEXT("Description"));
    int32 ItemTypeIndex = Headers.Find(TEXT("ItemType"));
    int32 ValueIndex = Headers.Find(TEXT("Value"));
    int32 WeightIndex = Headers.Find(TEXT("Weight"));
    int32 MaxStackSizeIndex = Headers.Find(TEXT("MaxStackSize"));
    int32 RarityIndex = Headers.Find(TEXT("Rarity"));
    int32 TagsIndex = Headers.Find(TEXT("Tags"));
    int32 EffectDesc1Index = Headers.Find(TEXT("EffectDescription1"));
    int32 EffectDesc2Index = Headers.Find(TEXT("EffectDescription2"));
    int32 IconPathIndex = Headers.Find(TEXT("IconPath"));
    int32 MeshPathIndex = Headers.Find(TEXT("MeshPath"));
    int32 VersionIndex = Headers.Find(TEXT("Version"));
    int32 LoadImmediatelyIndex = Headers.Find(TEXT("LoadImmediately"));
    
    // Check if all required columns are present
    if (AssetIdIndex == INDEX_NONE || DisplayNameIndex == INDEX_NONE || DescriptionIndex == INDEX_NONE)
    {
        UE_LOG(LogTemp, Error, TEXT("CSV file is missing required columns"));
        return false;
    }
    
    // Get the asset manager
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    
    // Ask if the user wants to immediately load imported assets
    bool bLoadAssetsImmediately = false;
    EAppReturnType::Type Response = FMessageDialog::Open(EAppMsgType::YesNo, 
        LOCTEXT("LoadAssetsQuestion", "Do you want to immediately load the imported assets? If no, assets will be registered but not loaded into memory."));
    
    if (Response == EAppReturnType::Yes)
    {
        bLoadAssetsImmediately = true;
    }
    
    // Counter for imported assets
    int32 ImportedCount = 0;
    
    // Process each data row
    for (int32 i = 1; i < Lines.Num(); ++i)
    {
        FString Line = Lines[i];
        if (Line.IsEmpty())
        {
            continue;
        }
        
        // Split the line into values, correctly handling quoted strings
        TArray<FString> Values;
        Values.Reserve(Headers.Num()); // Pre-allocate to avoid reallocations
        FString CurrentField;
        bool bInQuote = false;
        
        for (int32 CharIndex = 0; CharIndex < Line.Len(); ++CharIndex)
        {
            TCHAR CurrentChar = Line[CharIndex];
            
            if (CurrentChar == TEXT('"'))
            {
                bInQuote = !bInQuote;
            }
            else if (CurrentChar == TEXT(',') && !bInQuote)
            {
                Values.Add(CurrentField);
                CurrentField.Empty();
            }
            else
            {
                CurrentField.AppendChar(CurrentChar);
            }
        }
        
        // Add the last field
        Values.Add(CurrentField);
        
        // Make sure we have enough values
        if (Values.Num() <= FMath::Max3(AssetIdIndex, DisplayNameIndex, DescriptionIndex))
        {
            UE_LOG(LogTemp, Warning, TEXT("Skipping line %d: Not enough values"), i);
            continue;
        }
        
        // Extract data from the row
        FString AssetIdStr = Values[AssetIdIndex];
        FString DisplayName = Values[DisplayNameIndex];
        FString Description = Values[DescriptionIndex];
        
        // Remove quotes if present
        AssetIdStr = AssetIdStr.Replace(TEXT("\""), TEXT(""));
        DisplayName = DisplayName.Replace(TEXT("\""), TEXT(""));
        Description = Description.Replace(TEXT("\""), TEXT(""));
        
        // Create the asset ID
        FName AssetId = FName(*AssetIdStr);
        
        // Check if we already have this asset
        UCustomAssetBase* ExistingAsset = AssetManager.GetAssetById(AssetId);
        if (ExistingAsset)
        {
            UE_LOG(LogTemp, Warning, TEXT("Asset already exists with ID: %s"), *AssetIdStr);
            continue;
        }
        
        // Check if we should load this asset immediately
        bool bLoadThisAsset = bLoadAssetsImmediately;
        if (LoadImmediatelyIndex != INDEX_NONE && Values.Num() > LoadImmediatelyIndex)
        {
            FString LoadImmediatelyStr = Values[LoadImmediatelyIndex];
            LoadImmediatelyStr = LoadImmediatelyStr.Replace(TEXT("\""), TEXT(""));
            bLoadThisAsset = LoadImmediatelyStr.Equals(TEXT("true"), ESearchCase::IgnoreCase) || 
                            LoadImmediatelyStr.Equals(TEXT("1"), ESearchCase::IgnoreCase) ||
                            LoadImmediatelyStr.Equals(TEXT("yes"), ESearchCase::IgnoreCase);
        }
        
        if (bLoadThisAsset)
        {
            // Create a new item asset
            UCustomItemAsset* NewItemAsset = NewObject<UCustomItemAsset>();
            NewItemAsset->AssetId = AssetId;
            NewItemAsset->DisplayName = FText::FromString(DisplayName);
            NewItemAsset->Description = FText::FromString(Description);
            
            // Set additional properties if columns exist
            if (ItemTypeIndex != INDEX_NONE && Values.Num() > ItemTypeIndex)
            {
                FString ItemType = Values[ItemTypeIndex];
                ItemType = ItemType.Replace(TEXT("\""), TEXT(""));
                NewItemAsset->Category = FName(*ItemType);
            }
            
            if (ValueIndex != INDEX_NONE && Values.Num() > ValueIndex)
            {
                FString ValueStr = Values[ValueIndex];
                ValueStr = ValueStr.Replace(TEXT("\""), TEXT(""));
                if (!ValueStr.IsEmpty())
                {
                    NewItemAsset->Value = FCString::Atoi(*ValueStr);
                }
            }
            
            if (WeightIndex != INDEX_NONE && Values.Num() > WeightIndex)
            {
                FString WeightStr = Values[WeightIndex];
                WeightStr = WeightStr.Replace(TEXT("\""), TEXT(""));
                if (!WeightStr.IsEmpty())
                {
                    NewItemAsset->Weight = FCString::Atof(*WeightStr);
                }
            }
            
            if (MaxStackSizeIndex != INDEX_NONE && Values.Num() > MaxStackSizeIndex)
            {
                FString MaxStackStr = Values[MaxStackSizeIndex];
                MaxStackStr = MaxStackStr.Replace(TEXT("\""), TEXT(""));
                if (!MaxStackStr.IsEmpty())
                {
                    NewItemAsset->MaxStackSize = FCString::Atoi(*MaxStackStr);
                }
            }
            
            if (RarityIndex != INDEX_NONE && Values.Num() > RarityIndex)
            {
                FString Rarity = Values[RarityIndex];
                Rarity = Rarity.Replace(TEXT("\""), TEXT(""));
                NewItemAsset->Quality = StringToItemQuality(Rarity);
            }
            
            if (TagsIndex != INDEX_NONE && Values.Num() > TagsIndex)
            {
                FString TagsStr = Values[TagsIndex];
                TagsStr = TagsStr.Replace(TEXT("\""), TEXT(""));
                if (!TagsStr.IsEmpty())
                {
                    TArray<FString> TagArray;
                    TagsStr.ParseIntoArray(TagArray, TEXT(","), true);
                    
                    for (const FString& TagName : TagArray)
                    {
                        NewItemAsset->Tags.Add(FName(*TagName));
                    }
                }
            }
            
            if (EffectDesc1Index != INDEX_NONE && Values.Num() > EffectDesc1Index)
            {
                FString EffectDesc = Values[EffectDesc1Index];
                EffectDesc = EffectDesc.Replace(TEXT("\""), TEXT(""));
                if (!EffectDesc.IsEmpty())
                {
                    // Create and add a basic usage effect
                    FItemUsageEffect NewEffect;
                    NewEffect.StatName = FName(TEXT("Health"));
                    NewEffect.Value = 10.0f;  // Default healing value
                    NewEffect.Duration = 0.0f; // Instant effect
                    NewItemAsset->UsageEffects.Add(NewEffect);
                }
            }
            
            if (EffectDesc2Index != INDEX_NONE && Values.Num() > EffectDesc2Index)
            {
                FString EffectDesc = Values[EffectDesc2Index];
                EffectDesc = EffectDesc.Replace(TEXT("\""), TEXT(""));
                if (!EffectDesc.IsEmpty())
                {
                    // Create and add another basic usage effect (different stat)
                    FItemUsageEffect NewEffect;
                    NewEffect.StatName = FName(TEXT("Stamina"));
                    NewEffect.Value = 15.0f;  // Default stamina boost
                    NewEffect.Duration = 5.0f; // 5 second duration
                    NewItemAsset->UsageEffects.Add(NewEffect);
                }
            }
            
            if (IconPathIndex != INDEX_NONE && Values.Num() > IconPathIndex)
            {
                FString IconPath = Values[IconPathIndex];
                IconPath = IconPath.Replace(TEXT("\""), TEXT(""));
                if (!IconPath.IsEmpty())
                {
                    NewItemAsset->Icon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconPath));
                }
            }
            
            if (MeshPathIndex != INDEX_NONE && Values.Num() > MeshPathIndex)
            {
                FString MeshPath = Values[MeshPathIndex];
                MeshPath = MeshPath.Replace(TEXT("\""), TEXT(""));
                if (!MeshPath.IsEmpty())
                {
                    NewItemAsset->ItemMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(MeshPath));
                }
            }
            
            if (VersionIndex != INDEX_NONE && Values.Num() > VersionIndex)
            {
                FString VersionStr = Values[VersionIndex];
                VersionStr = VersionStr.Replace(TEXT("\""), TEXT(""));
                if (!VersionStr.IsEmpty())
                {
                    NewItemAsset->Version = FCString::Atoi(*VersionStr);
                }
            }
            
            // Register the asset with the asset manager
            AssetManager.RegisterAsset(NewItemAsset);
            UE_LOG(LogTemp, Log, TEXT("Created and loaded asset: %s"), *AssetIdStr);
        }
        else
        {
            // Just register the asset ID and path without loading it
            FSoftObjectPath AssetPath = FSoftObjectPath(FString::Printf(TEXT("/Game/Items/%s"), *AssetIdStr));
            AssetManager.RegisterAssetPath(AssetId, AssetPath);
            UE_LOG(LogTemp, Log, TEXT("Registered asset ID without loading: %s"), *AssetIdStr);
        }
        
        ImportedCount++;
    }
    
    // Show a notification with the number of imported assets
    if (ImportedCount > 0)
    {
        FString LoadStatus = bLoadAssetsImmediately ? TEXT("and loaded") : TEXT("without loading");
        FText NotificationText = FText::Format(
            LOCTEXT("AssetsImported", "Successfully imported {0} assets {1}"), 
            FText::AsNumber(ImportedCount),
            FText::FromString(LoadStatus)
        );
        
        FNotificationInfo Info(NotificationText);
        Info.ExpireDuration = 5.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
        
        return true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("No assets were imported from the CSV file"));
        return false;
    }
}

TSharedRef<SWidget> SCustomAssetManagerEditorWindow::CreateAssetFilterWidgets()
{
    // Initialize our static arrays for dropdown options if they're empty
    static TArray<TSharedPtr<FString>> TypeOptions;
    if (TypeOptions.Num() == 0)
    {
        TypeOptions.Add(MakeShared<FString>(TEXT("All Types")));
        TypeOptions.Add(MakeShared<FString>(TEXT("Item")));
        TypeOptions.Add(MakeShared<FString>(TEXT("Character")));
        TypeOptions.Add(MakeShared<FString>(TEXT("Weapon")));
        TypeOptions.Add(MakeShared<FString>(TEXT("Armor")));
        TypeOptions.Add(MakeShared<FString>(TEXT("Consumable")));
        TypeOptions.Add(MakeShared<FString>(TEXT("Material")));
        TypeOptions.Add(MakeShared<FString>(TEXT("Accessory")));
    }
    
    // Create an array of rarity options
    static TArray<TSharedPtr<FString>> RarityOptions;
    if (RarityOptions.Num() == 0)
    {
        RarityOptions.Add(MakeShared<FString>(TEXT("All Rarities")));
        RarityOptions.Add(MakeShared<FString>(TEXT("Common")));
        RarityOptions.Add(MakeShared<FString>(TEXT("Uncommon")));
        RarityOptions.Add(MakeShared<FString>(TEXT("Rare")));
        RarityOptions.Add(MakeShared<FString>(TEXT("Epic")));
        RarityOptions.Add(MakeShared<FString>(TEXT("Legendary")));
    }
    
    return SNew(SVerticalBox)
        
        // First row: Search box and type filter
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 0, 0, 5)
        [
            SNew(SHorizontalBox)
            
            // Search box
            + SHorizontalBox::Slot()
            .FillWidth(0.7f)
            .Padding(0, 0, 5, 0)
            [
                SAssignNew(AssetSearchBox, SSearchBox)
                .HintText(LOCTEXT("SearchAssetsHint", "Search assets..."))
                .OnTextChanged(this, &SCustomAssetManagerEditorWindow::OnAssetSearchTextChanged)
            ]
            
            // Type filter
            + SHorizontalBox::Slot()
            .FillWidth(0.3f)
            [
                SNew(SComboBox<TSharedPtr<FString>>)
                .OptionsSource(&TypeOptions)
                .OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewValue, ESelectInfo::Type) {
                    this->OnTypeFilterChanged_ComboBox(NewValue);
                })
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem) {
                    return SNew(STextBlock).Text(FText::FromString(*InItem));
                })
                [
                    SNew(STextBlock)
                    .Text_Lambda([this]() {
                        return FText::FromString(TypeFilter.IsEmpty() ? TEXT("All Types") : TypeFilter);
                    })
                ]
            ]
        ]
        
        // Second row: Rarity filter and reset button
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SHorizontalBox)
            
            // Rarity filter
            + SHorizontalBox::Slot()
            .FillWidth(0.3f)
            .Padding(0, 0, 5, 0)
            [
                SNew(SComboBox<TSharedPtr<FString>>)
                .OptionsSource(&RarityOptions)
                .OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewValue, ESelectInfo::Type) {
                    this->OnRarityFilterChanged_ComboBox(NewValue);
                })
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem) {
                    return SNew(STextBlock).Text(FText::FromString(*InItem));
                })
                [
                    SNew(STextBlock)
                    .Text_Lambda([this]() {
                        return FText::FromString(RarityFilter.IsEmpty() ? TEXT("All Rarities") : RarityFilter);
                    })
                ]
            ]
            
            // Reset filters button
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(5, 0, 0, 0)
            [
                SNew(SButton)
                .ContentPadding(FMargin(8, 2))
                .Text(LOCTEXT("ResetFilters", "Reset Filters"))
                .OnClicked(this, &SCustomAssetManagerEditorWindow::OnResetFiltersClicked)
            ]
        ];
}

void SCustomAssetManagerEditorWindow::ApplyFilters()
{
    FilteredAssetEntries.Empty();
    
    for (TSharedPtr<FAssetEntry> AssetEntry : AssetEntries)
    {
        bool bPassesSearch = AssetSearchString.IsEmpty() || 
            AssetEntry->DisplayName.ToString().Contains(AssetSearchString) || 
            AssetEntry->AssetId.ToString().Contains(AssetSearchString) ||
            AssetEntry->Description.ToString().Contains(AssetSearchString);
        
        bool bPassesTypeFilter = TypeFilter.IsEmpty() || AssetEntry->AssetType.Contains(TypeFilter);
        
        bool bPassesRarityFilter = RarityFilter.IsEmpty() || 
            (AssetEntry->ItemAsset && GetEnumValueString(AssetEntry->ItemAsset->Quality).Contains(RarityFilter));
        
        bool bPassesBundleFilter = BundleFilter.IsNone() || AssetEntry->Bundles.Contains(BundleFilter);
        
        if (bPassesSearch && bPassesTypeFilter && bPassesRarityFilter && bPassesBundleFilter)
        {
            FilteredAssetEntries.Add(AssetEntry);
        }
    }
    
    if (AssetListView.IsValid())
    {
        AssetListView->RequestListRefresh();
    }
}

void SCustomAssetManagerEditorWindow::OnAssetSearchTextChanged(const FText& NewText)
{
    AssetSearchString = NewText.ToString();
    ApplyFilters();
}

void SCustomAssetManagerEditorWindow::OnTypeFilterChanged_ComboBox(TSharedPtr<FString> NewValue)
{
    if (NewValue.IsValid())
    {
        if (*NewValue == "All Types")
        {
            TypeFilter.Empty();
        }
        else
        {
            TypeFilter = *NewValue;
        }
        ApplyFilters();
    }
}

void SCustomAssetManagerEditorWindow::OnRarityFilterChanged_ComboBox(TSharedPtr<FString> NewValue)
{
    if (NewValue.IsValid())
    {
        if (*NewValue == "All Rarities")
        {
            RarityFilter.Empty();
        }
        else
        {
            RarityFilter = *NewValue;
        }
        ApplyFilters();
    }
}

void SCustomAssetManagerEditorWindow::OnBundleFilterChanged_ComboBox(TSharedPtr<FBundleEntry> NewValue)
{
    if (NewValue.IsValid() && NewValue->Bundle)
    {
        BundleFilter = NewValue->Bundle->BundleId;
        ApplyFilters();
    }
}

FReply SCustomAssetManagerEditorWindow::OnResetFiltersClicked()
{
    AssetSearchString.Empty();
    TypeFilter.Empty();
    RarityFilter.Empty();
    BundleFilter = FName(NAME_None);
    ApplyFilters();
    return FReply::Handled();
}

FLinearColor SCustomAssetManagerEditorWindow::GetAssetTypeColor(const FString& AssetType) const
{
    if (AssetType == TEXT("Item"))
    {
        return FLinearColor(0.2f, 0.8f, 0.2f); // Green for items
    }
    else if (AssetType == TEXT("Character"))
    {
        return FLinearColor(0.2f, 0.2f, 0.8f); // Blue for characters
    }
    else if (AssetType == TEXT("Unknown"))
    {
        return FLinearColor(0.5f, 0.5f, 0.5f); // Gray for unknown
    }
    
    return FLinearColor::White; // Default color
}

// Helper function to convert string to EItemQuality
EItemQuality StringToItemQuality(const FString& QualityStr)
{
    if (QualityStr.Equals(TEXT("Common"), ESearchCase::IgnoreCase))
        return EItemQuality::Common;
    else if (QualityStr.Equals(TEXT("Uncommon"), ESearchCase::IgnoreCase))
        return EItemQuality::Uncommon;
    else if (QualityStr.Equals(TEXT("Rare"), ESearchCase::IgnoreCase))
        return EItemQuality::Rare;
    else if (QualityStr.Equals(TEXT("Epic"), ESearchCase::IgnoreCase))
        return EItemQuality::Epic;
    else if (QualityStr.Equals(TEXT("Legendary"), ESearchCase::IgnoreCase))
        return EItemQuality::Legendary;
    else if (QualityStr.Equals(TEXT("Unique"), ESearchCase::IgnoreCase))
        return EItemQuality::Unique;
    else
        return EItemQuality::Common; // Default
}

// Helper function to get enum value as string
FString GetEnumValueString(const EItemQuality Quality)
{
    switch (Quality)
    {
        case EItemQuality::Common:     return TEXT("Common");
        case EItemQuality::Uncommon:   return TEXT("Uncommon");
        case EItemQuality::Rare:       return TEXT("Rare");
        case EItemQuality::Epic:       return TEXT("Epic");
        case EItemQuality::Legendary:  return TEXT("Legendary");
        case EItemQuality::Unique:     return TEXT("Unique");
        default:                       return TEXT("Unknown");
    }
}

// Helper function to get enum value as string for ECharacterClass
FString GetEnumValueString(const ECharacterClass CharClass)
{
    switch (CharClass)
    {
        case ECharacterClass::Warrior:  return TEXT("Warrior");
        case ECharacterClass::Ranger:   return TEXT("Ranger");
        case ECharacterClass::Mage:     return TEXT("Mage");
        case ECharacterClass::Rogue:    return TEXT("Rogue");
        case ECharacterClass::Support:  return TEXT("Support");
        case ECharacterClass::Monster:  return TEXT("Monster");
        case ECharacterClass::NPC:      return TEXT("NPC");
        default:                        return TEXT("Unknown");
    }
}

void SCustomAssetManagerEditorWindow::ShowRenameBundleDialog(UCustomAssetBundle* Bundle)
{
    if (!Bundle)
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowRenameBundleDialog called with null bundle"));
        return;
    }

    // Create a dialog to get the new bundle name
    TSharedRef<SEditableTextBox> BundleNameTextBox = SNew(SEditableTextBox)
        .Text(Bundle->DisplayName)
        .HintText(LOCTEXT("BundleRenameHint", "Enter new bundle name"))
        .MinDesiredWidth(250);
        
    // Show dialog - use TSharedPtr instead of TSharedRef to better handle lifetime
    TSharedPtr<SWindow> DialogWindow = SNew(SWindow)
        .Title(LOCTEXT("RenameBundleTitle", "Rename Asset Bundle"))
        .ClientSize(FVector2D(400, 120))
        .SupportsMaximize(false)
        .SupportsMinimize(false);
        
    DialogWindow->SetContent(
        SNew(SVerticalBox)
        +SVerticalBox::Slot()
        .Padding(10)
        .AutoHeight()
        [
            SNew(STextBlock)
            .Text(FText::Format(LOCTEXT("RenameBundlePrompt", "Rename bundle '{0}' to:"), Bundle->DisplayName))
        ]
        +SVerticalBox::Slot()
        .Padding(10)
        .AutoHeight()
        [
            BundleNameTextBox
        ]
        +SVerticalBox::Slot()
        .Padding(10)
        .AutoHeight()
        .HAlign(HAlign_Right)
        [
            SNew(SHorizontalBox)
            +SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("RenameButton", "Rename"))
                .OnClicked_Lambda([this, BundleNameTextBox, Bundle, WeakDialogWindow = TWeakPtr<SWindow>(DialogWindow)]()
                {
                    FString NewBundleName = BundleNameTextBox->GetText().ToString();
                    if (NewBundleName.IsEmpty())
                    {
                        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("EmptyBundleName", "Bundle name cannot be empty."));
                        return FReply::Handled();
                    }
                    
                    // Get the asset manager
                    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
                    
                    // Rename the bundle
                    bool bSuccess = AssetManager.RenameBundle(Bundle->BundleId, NewBundleName);
                    
                    if (bSuccess)
                    {
                        UE_LOG(LogTemp, Log, TEXT("Bundle renamed successfully"));
                        
                        // Close dialog
                        TSharedPtr<SWindow> PinnedWindow = WeakDialogWindow.Pin();
                        if (PinnedWindow.IsValid())
                        {
                            FSlateApplication::Get().RequestDestroyWindow(PinnedWindow.ToSharedRef());
                        }
                        
                        // Refresh the bundle list
                        RefreshBundleList();
                    }
                    else
                    {
                        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("FailedToRename", "Failed to rename bundle."));
                    }
                    
                    return FReply::Handled();
                })
            ]
            +SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("CancelButton", "Cancel"))
                .OnClicked_Lambda([WeakDialogWindow = TWeakPtr<SWindow>(DialogWindow)]()
                {
                    TSharedPtr<SWindow> PinnedWindow = WeakDialogWindow.Pin();
                    if (PinnedWindow.IsValid())
                    {
                        FSlateApplication::Get().RequestDestroyWindow(PinnedWindow.ToSharedRef());
                    }
                    return FReply::Handled();
                })
            ]
        ]
    );
    
    FSlateApplication::Get().AddModalWindow(DialogWindow.ToSharedRef(), FGlobalTabmanager::Get()->GetRootWindow());
}

void SCustomAssetManagerEditorWindow::ShowDeleteBundleDialog(UCustomAssetBundle* Bundle)
{
    if (!Bundle)
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowDeleteBundleDialog called with null bundle"));
        return;
    }

    // Create confirmation dialog
    EAppReturnType::Type ReturnType = FMessageDialog::Open(
        EAppMsgType::YesNo, 
        FText::Format(
            LOCTEXT("DeleteBundleConfirmation", "Are you sure you want to delete the bundle '{0}'?\nThis will not delete the assets in the bundle."), 
            Bundle->DisplayName
        )
    );
    
    if (ReturnType == EAppReturnType::Yes)
    {
        // Get the asset manager
        UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
        
        // Delete the bundle
        bool bSuccess = AssetManager.DeleteBundle(Bundle->BundleId);
        
        if (bSuccess)
        {
            UE_LOG(LogTemp, Log, TEXT("Bundle deleted successfully"));
            
            // Refresh the bundle list
            RefreshBundleList();
            
            // Also refresh the asset list to update bundle membership
            RefreshAssetList();
        }
        else
        {
            FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("FailedToDelete", "Failed to delete bundle."));
        }
    }
}

void SCustomAssetManagerEditorWindow::ShowRemoveAssetsFromSelectedBundleDialog()
{
    if (SelectedBundleId.IsNone())
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoBundleSelected", "No bundle selected. Please select a bundle first."));
        return;
    }

    // Get the asset manager
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    
    // Get the selected bundle
    UCustomAssetBundle* SelectedBundle = AssetManager.GetBundleById(SelectedBundleId);
    if (!SelectedBundle)
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("BundleNotFound", "Selected bundle not found."));
        return;
    }
    
    // Check if the bundle has any assets
    if (SelectedBundle->AssetIds.Num() == 0)
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("BundleEmpty", "This bundle does not contain any assets."));
        return;
    }
    
    // Create a dialog to show assets in the bundle
    TSharedRef<SWindow> DialogWindow = SNew(SWindow)
        .Title(FText::Format(LOCTEXT("RemoveAssetsFromBundleTitle", "Remove Assets from Bundle: {0}"), SelectedBundle->DisplayName))
        .ClientSize(FVector2D(500, 400))
        .SupportsMaximize(false)
        .SupportsMinimize(false);
        
    // Create a list of assets in the bundle
    TArray<TSharedPtr<FAssetListItem>> AssetOptions;
    for (const FName& AssetId : SelectedBundle->AssetIds)
    {
        TSharedPtr<FAssetListItem> Item = MakeShared<FAssetListItem>();
        Item->AssetId = AssetId;
        
        // Get display name for the asset (either from the loaded asset or from the ID)
        UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
        Item->DisplayName = Asset ? Asset->DisplayName : FText::FromName(AssetId);
        
        AssetOptions.Add(Item);
    }
    
    // Create a list view for the assets with multi-selection
    TSharedPtr<SListView<TSharedPtr<FAssetListItem>>> AssetSelectionListView;
    
    // Track the selected assets
    TArray<TSharedPtr<FAssetListItem>> SelectedAssets;
    
    DialogWindow->SetContent(
        SNew(SVerticalBox)
        +SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        [
            SNew(STextBlock)
            .Text(FText::Format(LOCTEXT("SelectAssetsToRemove", "Select assets to remove from bundle '{0}':"), SelectedBundle->DisplayName))
        ]
        +SVerticalBox::Slot()
        .FillHeight(1.0f)
        .Padding(10)
        [
            SAssignNew(AssetSelectionListView, SListView<TSharedPtr<FAssetListItem>>)
            .ListItemsSource(&AssetOptions)
            .SelectionMode(ESelectionMode::Multi)
            .OnGenerateRow_Lambda([](TSharedPtr<FAssetListItem> Item, const TSharedRef<STableViewBase>& OwnerTable) {
                return SNew(STableRow<TSharedPtr<FAssetListItem>>, OwnerTable)
                [
                    SNew(STextBlock)
                    .Text(Item->DisplayName)
                ];
            })
            .OnSelectionChanged_Lambda([&SelectedAssets](TSharedPtr<FAssetListItem> Item, ESelectInfo::Type SelectType) {
                // Get all selected items from the list view
                // Note: This will be handled in the remove button click handler
            })
        ]
        +SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        .HAlign(HAlign_Right)
        [
            SNew(SHorizontalBox)
            +SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("RemoveButton", "Remove Selected"))
                .OnClicked_Lambda([this, DialogWindow, AssetSelectionListView, SelectedBundle, &AssetManager]() {
                    // Get the selected assets
                    TArray<TSharedPtr<FAssetListItem>> SelectedItems = AssetSelectionListView->GetSelectedItems();
                    
                    if (SelectedItems.Num() == 0)
                    {
                        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoAssetsSelected", "No assets selected for removal. Please select one or more assets."));
                        return FReply::Handled();
                    }
                    
                    // Confirm with the user
                    FText Message = FText::Format(
                        LOCTEXT("ConfirmAssetRemoval", "Are you sure you want to remove the selected {0} assets from bundle '{1}'?"),
                        FText::AsNumber(SelectedItems.Num()),
                        SelectedBundle->DisplayName
                    );
                    
                    FText Title = LOCTEXT("ConfirmRemovalTitle", "Confirm Asset Removal");
                    
                    if (FMessageDialog::Open(EAppMsgType::YesNo, Message, &Title) == EAppReturnType::Yes)
                    {
                        // Remove all selected assets from the bundle
                        int32 RemovedCount = 0;
                        for (const TSharedPtr<FAssetListItem>& Item : SelectedItems)
                        {
                            // Skip invalid assets
                            if (Item->AssetId.IsNone())
                            {
                                continue;
                            }
                            
                            UE_LOG(LogTemp, Log, TEXT("Removing asset %s from bundle %s"), 
                                *Item->AssetId.ToString(), *SelectedBundle->BundleId.ToString());
                                
                            // Remove the asset from the bundle
                            SelectedBundle->RemoveAsset(Item->AssetId);
                            RemovedCount++;
                        }
                        
                        // Save the bundle
                        try
                        {
                            bool bSaved = SelectedBundle->Save();
                            
                            if (!bSaved)
                            {
                                UE_LOG(LogTemp, Error, TEXT("Failed to save bundle %s after removing assets"), *SelectedBundle->BundleId.ToString());
                                FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("FailedToSaveBundle", 
                                    "Failed to save bundle {0}. Check the logs for more information."), SelectedBundle->DisplayName));
                            }
                            
                            // Refresh UI
                            RefreshAssetList();
                            RefreshBundleList();
                            
                            // Show success message
                            FMessageDialog::Open(EAppMsgType::Ok, 
                                FText::Format(LOCTEXT("AssetsRemovedFromBundle", "{0} assets removed from bundle '{1}'"), 
                                FText::AsNumber(RemovedCount), 
                                SelectedBundle->DisplayName));
                        }
                        catch (const std::exception& e)
                        {
                            UE_LOG(LogTemp, Error, TEXT("Exception when saving bundle after asset removal: %s"), UTF8_TO_TCHAR(e.what()));
                            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("Error saving bundle: %s"), UTF8_TO_TCHAR(e.what()))));
                        }
                    }
                    
                    // Close the dialog
                    FSlateApplication::Get().RequestDestroyWindow(DialogWindow);
                    return FReply::Handled();
                })
            ]
            +SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("CancelButton", "Cancel"))
                .OnClicked_Lambda([DialogWindow]() {
                    FSlateApplication::Get().RequestDestroyWindow(DialogWindow);
                    return FReply::Handled();
                })
            ]
        ]
    );
    
    FSlateApplication::Get().AddModalWindow(DialogWindow, FGlobalTabmanager::Get()->GetRootWindow());
}

#undef LOCTEXT_NAMESPACE 