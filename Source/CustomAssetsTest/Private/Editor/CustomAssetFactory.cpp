#include "Editor/CustomAssetFactory.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Layout/SBox.h"
#include "Styling/AppStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Assets/CustomAssetBase.h"
#include "Assets/CustomAssetBundle.h"

#define LOCTEXT_NAMESPACE "CustomAssetFactory"

//////////////////////////////////////////////////////////////////////////
// SCustomAssetCreationDialog

void SCustomAssetCreationDialog::Construct(const FArguments& InArgs)
{
    bWasConfirmed = false;
    
    ChildSlot
    [
        SNew(SVerticalBox)
        
        // Asset ID
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("AssetIdLabel", "Asset ID:"))
                .ToolTipText(LOCTEXT("AssetIdTooltip", "Unique identifier for this asset"))
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SAssignNew(AssetIdTextBox, SEditableTextBox)
                .Text(FText::FromString(TEXT("Asset_")))
                .ToolTipText(LOCTEXT("AssetIdTooltip", "Unique identifier for this asset"))
            ]
        ]
        
        // Display Name
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("DisplayNameLabel", "Display Name:"))
                .ToolTipText(LOCTEXT("DisplayNameTooltip", "Human-readable name for this asset"))
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SAssignNew(DisplayNameTextBox, SEditableTextBox)
                .Text(FText::FromString(TEXT("New Custom Asset")))
                .ToolTipText(LOCTEXT("DisplayNameTooltip", "Human-readable name for this asset"))
            ]
        ]
        
        // Description
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("DescriptionLabel", "Description:"))
            .ToolTipText(LOCTEXT("DescriptionTooltip", "Description of this asset"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SAssignNew(DescriptionTextBox, SMultiLineEditableTextBox)
            .Text(FText::FromString(TEXT("")))
            .ToolTipText(LOCTEXT("DescriptionTooltip", "Description of this asset"))
        ]
        
        // Tags
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("TagsLabel", "Tags:"))
            .ToolTipText(LOCTEXT("TagsTooltip", "Tags for categorizing this asset"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SAssignNew(TagTextBox, SEditableTextBox)
                .HintText(LOCTEXT("TagHint", "Enter a tag and press Enter"))
                .OnTextCommitted(this, &SCustomAssetCreationDialog::OnTagCommitted)
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .MaxHeight(100)
        .Padding(5)
        [
            SAssignNew(TagsListView, SListView<TSharedPtr<FName>>)
            .ItemHeight(20)
            .ListItemsSource(&TagEntries)
            .OnGenerateRow(this, &SCustomAssetCreationDialog::OnGenerateTagRow)
        ]
        
        // Buttons
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        .HAlign(HAlign_Right)
        [
            SNew(SUniformGridPanel)
            .SlotPadding(FMargin(5))
            + SUniformGridPanel::Slot(0, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("OkButton", "OK"))
                .OnClicked(this, &SCustomAssetCreationDialog::OnOkClicked)
            ]
            + SUniformGridPanel::Slot(1, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("CancelButton", "Cancel"))
                .OnClicked(this, &SCustomAssetCreationDialog::OnCancelClicked)
            ]
        ]
    ];
}

FName SCustomAssetCreationDialog::GetAssetId() const
{
    return FName(*AssetIdTextBox->GetText().ToString());
}

FText SCustomAssetCreationDialog::GetDisplayName() const
{
    return DisplayNameTextBox->GetText();
}

FText SCustomAssetCreationDialog::GetDescription() const
{
    return DescriptionTextBox->GetText();
}

TArray<FName> SCustomAssetCreationDialog::GetTags() const
{
    return Tags;
}

bool SCustomAssetCreationDialog::ShowDialog()
{
    // Create the window
    ParentWindow = SNew(SWindow)
        .Title(LOCTEXT("CustomAssetCreationDialogTitle", "Create Custom Asset"))
        .ClientSize(FVector2D(400, 400))
        .SizingRule(ESizingRule::FixedSize)
        .SupportsMaximize(false)
        .SupportsMinimize(false);
    
    // Add this widget as the window's content
    ParentWindow->SetContent(
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        [
            SharedThis(this)
        ]
    );
    
    // Show the window
    FSlateApplication::Get().AddModalWindow(ParentWindow.ToSharedRef(), nullptr);
    
    return bWasConfirmed;
}

FReply SCustomAssetCreationDialog::OnOkClicked()
{
    // Validate the input
    if (AssetIdTextBox->GetText().IsEmpty())
    {
        // Show an error message
        // FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("EmptyAssetIdError", "Asset ID cannot be empty"));
        return FReply::Handled();
    }
    
    if (DisplayNameTextBox->GetText().IsEmpty())
    {
        // Show an error message
        // FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("EmptyDisplayNameError", "Display name cannot be empty"));
        return FReply::Handled();
    }
    
    // Set the result and close the window
    bWasConfirmed = true;
    ParentWindow->RequestDestroyWindow();
    
    return FReply::Handled();
}

FReply SCustomAssetCreationDialog::OnCancelClicked()
{
    // Close the window without confirming
    bWasConfirmed = false;
    ParentWindow->RequestDestroyWindow();
    
    return FReply::Handled();
}

void SCustomAssetCreationDialog::OnTagCommitted(const FText& TagText, ETextCommit::Type CommitType)
{
    if (CommitType != ETextCommit::OnEnter || TagText.IsEmpty())
    {
        return;
    }
    
    // Add the tag if it doesn't already exist
    FName NewTag = FName(*TagText.ToString());
    if (!Tags.Contains(NewTag))
    {
        Tags.Add(NewTag);
        TagEntries.Add(MakeShared<FName>(NewTag));
        TagsListView->RequestListRefresh();
    }
    
    // Clear the text box
    TagTextBox->SetText(FText::GetEmpty());
}

TSharedRef<ITableRow> SCustomAssetCreationDialog::OnGenerateTagRow(TSharedPtr<FName> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(STableRow<TSharedPtr<FName>>, OwnerTable)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(FText::FromName(*Item))
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(5, 0, 0, 0)
        [
            SNew(SButton)
            .ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
            .OnClicked_Lambda([this, Item]() {
                this->RemoveTag(*Item);
                return FReply::Handled();
            })
            .ToolTipText(LOCTEXT("RemoveTagTooltip", "Remove this tag"))
            .ContentPadding(0)
            [
                SNew(SImage)
                .Image(FAppStyle::GetBrush("Icons.Delete"))
            ]
        ]
    ];
}

void SCustomAssetCreationDialog::RemoveTag(const FName& TagToRemove)
{
    // Remove the tag
    Tags.Remove(TagToRemove);
    
    // Remove the entry from the list
    for (int32 i = 0; i < TagEntries.Num(); ++i)
    {
        if (*TagEntries[i] == TagToRemove)
        {
            TagEntries.RemoveAt(i);
            break;
        }
    }
    
    // Refresh the list
    TagsListView->RequestListRefresh();
}

#undef LOCTEXT_NAMESPACE 