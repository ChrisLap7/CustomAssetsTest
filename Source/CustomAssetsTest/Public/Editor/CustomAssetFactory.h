#pragma once

#include "CoreMinimal.h"
#include "Assets/CustomAssetBase.h"
#include "Assets/CustomItemAsset.h"
#include "Assets/CustomCharacterAsset.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

// Forward declarations
class UFactory;
class FFeedbackContext;

/**
 * Dialog for configuring a new custom asset
 */
class SCustomAssetCreationDialog : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCustomAssetCreationDialog) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    /** Gets the asset ID */
    FName GetAssetId() const;

    /** Gets the display name */
    FText GetDisplayName() const;

    /** Gets the description */
    FText GetDescription() const;

    /** Gets the tags */
    TArray<FName> GetTags() const;

    /** Shows the dialog and returns true if the user clicked OK */
    bool ShowDialog();

private:
    /** Handler for when the OK button is clicked */
    FReply OnOkClicked();

    /** Handler for when the Cancel button is clicked */
    FReply OnCancelClicked();

    /** Handler for when a tag is committed */
    void OnTagCommitted(const FText& TagText, ETextCommit::Type CommitType);
    
    /** Handler for generating a row in the tag list */
    TSharedRef<ITableRow> OnGenerateTagRow(TSharedPtr<FName> Item, const TSharedRef<STableViewBase>& OwnerTable);
    
    /** Removes a tag */
    void RemoveTag(const FName& TagToRemove);

    /** The window containing this widget */
    TSharedPtr<SWindow> ParentWindow;

    /** The asset ID text */
    TSharedPtr<SEditableTextBox> AssetIdTextBox;

    /** The display name text */
    TSharedPtr<SEditableTextBox> DisplayNameTextBox;

    /** The description text */
    TSharedPtr<SMultiLineEditableTextBox, ESPMode::ThreadSafe> DescriptionTextBox;

    /** The tags text */
    TSharedPtr<SEditableTextBox> TagTextBox;

    /** The list of tags */
    TArray<FName> Tags;

    /** The tags list view */
    TSharedPtr<SListView<TSharedPtr<FName>>> TagsListView;

    /** The tags data */
    TArray<TSharedPtr<FName>> TagEntries;

    /** Whether the dialog was confirmed */
    bool bWasConfirmed;
}; 