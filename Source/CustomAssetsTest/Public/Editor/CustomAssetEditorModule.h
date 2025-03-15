#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Framework/Commands/UICommandList.h"

#define LOCTEXT_NAMESPACE "CustomAssetEditorModule"

/**
 * Module for editor functionality of the Custom Asset System
 */
class FCustomAssetEditorModule : public IModuleInterface
{
public:
    // IModuleInterface implementation
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    // Function to open the asset manager window
    void OnOpenAssetManager();

private:
    // Helper method to register menus using the ToolMenus system
    void RegisterMenus();

    // Command list for editor actions
    TSharedPtr<FUICommandList> PluginCommands;
};

#undef LOCTEXT_NAMESPACE 