#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Styling/AppStyle.h"

class FCustomAssetManagerCommands : public TCommands<FCustomAssetManagerCommands>
{
public:
    FCustomAssetManagerCommands();

    // TCommands<> interface
    virtual void RegisterCommands() override;

public:
    // Command for opening the asset manager window
    TSharedPtr<FUICommandInfo> OpenAssetManagerWindow;

public:
    // Helper functions
    static const FCustomAssetManagerCommands& Get();
    static void Register();
    static void Unregister();
}; 