#include "Editor/CustomAssetManagerCommands.h"

#define LOCTEXT_NAMESPACE "CustomAssetManagerCommands"

FCustomAssetManagerCommands::FCustomAssetManagerCommands()
    : TCommands<FCustomAssetManagerCommands>(TEXT("CustomAssetManager"), 
        NSLOCTEXT("Contexts", "CustomAssetManager", "Custom Asset Manager"), 
        NAME_None, 
        FAppStyle::GetAppStyleSetName())
{
}

void FCustomAssetManagerCommands::RegisterCommands()
{
    UI_COMMAND(OpenAssetManagerWindow, "Custom Asset Manager", "Open the Custom Asset Manager window", EUserInterfaceActionType::Button, FInputChord());
}

const FCustomAssetManagerCommands& FCustomAssetManagerCommands::Get()
{
    return TCommands<FCustomAssetManagerCommands>::Get();
}

void FCustomAssetManagerCommands::Register()
{
    TCommands<FCustomAssetManagerCommands>::Register();
}

void FCustomAssetManagerCommands::Unregister()
{
    TCommands<FCustomAssetManagerCommands>::Unregister();
}

#undef LOCTEXT_NAMESPACE 