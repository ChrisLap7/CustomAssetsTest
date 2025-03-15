#include "Editor/CustomAssetEditorModule.h"
#include "Modules/ModuleManager.h"
#include "Editor/CustomAssetManagerEditorWindow.h"
#include "LevelEditor.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"

// In UE5.4, this module needs a different implementation approach
// For now, just provide a minimal implementation to get things compiling

#define LOCTEXT_NAMESPACE "CustomAssetEditorModule"

void FCustomAssetEditorModule::StartupModule()
{
    // Add very prominent debug logging to see if this function is called
    UE_LOG(LogTemp, Warning, TEXT("==================================================="));
    UE_LOG(LogTemp, Warning, TEXT("=== CustomAssetEditorModule: StartupModule CALLED ==="));
    UE_LOG(LogTemp, Warning, TEXT("==================================================="));
    
    if (!IsRunningCommandlet())
    {
        // Register the UI commands
        FCustomAssetManagerCommands::Register();
        
        // Create the command list
        PluginCommands = MakeShareable(new FUICommandList);
        
        // Map commands to actions
        PluginCommands->MapAction(
            FCustomAssetManagerCommands::Get().OpenAssetManagerWindow,
            FExecuteAction::CreateRaw(this, &FCustomAssetEditorModule::OnOpenAssetManager),
            FCanExecuteAction());
        
        // Add menu extension using the newer ToolMenus API
        UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FCustomAssetEditorModule::RegisterMenus));
        
        UE_LOG(LogTemp, Warning, TEXT("CustomAssetEditorModule: Commands registered, menu callback set up"));
    }
}

void FCustomAssetEditorModule::ShutdownModule()
{
    if (UToolMenus::IsToolMenuUIEnabled())
    {
        UToolMenus::UnregisterOwner(this);
    }
    
    // Unregister commands
    FCustomAssetManagerCommands::Unregister();
    
    UE_LOG(LogTemp, Display, TEXT("CustomAssetEditorModule: ShutdownModule - Editor module shutdown"));
}

void FCustomAssetEditorModule::RegisterMenus()
{
    // Add strong debug logging
    UE_LOG(LogTemp, Warning, TEXT("=================================================="));
    UE_LOG(LogTemp, Warning, TEXT("=== CustomAssetEditorModule: RegisterMenus CALLED ==="));
    UE_LOG(LogTemp, Warning, TEXT("=================================================="));
    
    // Add the menu entry to the Window menu
    FToolMenuOwnerScoped OwnerScoped(this);
    
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    if (!Menu)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find Window menu!"));
        return;
    }
    
    FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
    
    Section.AddMenuEntryWithCommandList(
        FCustomAssetManagerCommands::Get().OpenAssetManagerWindow,
        PluginCommands,
        LOCTEXT("AssetManagerMenuEntry", "Custom Asset Manager"),
        LOCTEXT("AssetManagerMenuEntryTooltip", "Open the Custom Asset Manager window"),
        FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.TextRenderComponent")
    );
    
    UE_LOG(LogTemp, Warning, TEXT("CustomAssetEditorModule: Menu entry successfully added to Window menu"));
}

void FCustomAssetEditorModule::OnOpenAssetManager()
{
    UE_LOG(LogTemp, Display, TEXT("Opening Custom Asset Manager window"));
    SCustomAssetManagerEditorWindow::OpenWindow();
}

void FCustomAssetEditorModule::AddMenuEntry(FMenuBuilder& MenuBuilder)
{
    // Deprecated - using RegisterMenus instead
    MenuBuilder.AddMenuEntry(FCustomAssetManagerCommands::Get().OpenAssetManagerWindow);
}

#undef LOCTEXT_NAMESPACE 

// We need to use a unique module name for the editor module - do not reimplement the main module
// Remove the IMPLEMENT_MODULE line completely since this class should be part of the main module
// The main module already has IMPLEMENT_PRIMARY_GAME_MODULE in CustomAssetsTest.cpp 