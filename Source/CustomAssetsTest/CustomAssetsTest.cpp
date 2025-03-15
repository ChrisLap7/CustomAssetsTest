// Copyright Epic Games, Inc. All Rights Reserved.

#include "CustomAssetsTest.h"
#include "Modules/ModuleManager.h"
#include "Assets/CustomAssetManager.h"
#include "Engine/Engine.h"
#include "Engine/GameEngine.h"
#include "GameFramework/GameModeBase.h"
#include "Editor/CustomAssetEditorModule.h"

// Main game module implementation - extends the default implementation to load our editor module
class FCustomAssetsTestGameModule : public FDefaultGameModuleImpl
{
public:
    // Called when the module is first loaded
    virtual void StartupModule() override
    {
        FDefaultGameModuleImpl::StartupModule();

        // Initialize editor module directly if in editor
#if WITH_EDITOR
        // Create and initialize our editor module
        EditorModule = MakeShareable(new FCustomAssetEditorModule());
        EditorModule->StartupModule();
        
        UE_LOG(LogCustomAssetsTest, Display, TEXT("CustomAssetsTest: Editor module initialized from game module"));
#endif
    }

    // Called before the module is unloaded
    virtual void ShutdownModule() override
    {
#if WITH_EDITOR
        if (EditorModule.IsValid())
        {
            EditorModule->ShutdownModule();
            EditorModule.Reset();
        }
#endif

        FDefaultGameModuleImpl::ShutdownModule();
    }

private:
#if WITH_EDITOR
    // Reference to our editor module
    TSharedPtr<FCustomAssetEditorModule> EditorModule;
#endif
};

// Register our custom module implementation instead of the default one
IMPLEMENT_PRIMARY_GAME_MODULE(FCustomAssetsTestGameModule, CustomAssetsTest, "CustomAssetsTest");

DEFINE_LOG_CATEGORY(LogCustomAssetsTest)

// Register the asset manager factory function directly as a creation function
// In UE5 we don't need code to register the asset manager, it's handled via DefaultEngine.ini:
// [/Script/Engine.Engine]
// AssetManagerClassName=/Script/CustomAssetsTest.CustomAssetManager
 