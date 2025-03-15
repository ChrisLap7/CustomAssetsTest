#include "Assets/CustomItemAsset.h"
#include "UObject/ConstructorHelpers.h"

UCustomItemAsset::UCustomItemAsset()
{
    // Initialize default values
    Value = 0;
    Weight = 1.0f;
    Quality = EItemQuality::Common;
    Category = TEXT("Miscellaneous");
    bStackable = false;
    MaxStackSize = 1;
    bConsumable = false;
    RequiredLevel = 1;
    Cooldown = 0.0f;
    bUseLOD = false;
    LODSwitchDistance = 2000.0f;
}

bool UCustomItemAsset::CanBeUsedBy(const TMap<FName, float>& EntityStats) const
{
    // Check level requirement
    if (EntityStats.Contains(TEXT("Level")))
    {
        float EntityLevel = EntityStats.FindRef(TEXT("Level"));
        if (EntityLevel < RequiredLevel)
        {
            return false;
        }
    }
    else if (RequiredLevel > 1)
    {
        // If no level stat is provided and we have a level requirement, fail
        return false;
    }

    // Check attribute requirements
    for (const auto& ReqAttribute : RequiredAttributes)
    {
        float EntityAttributeValue = EntityStats.Contains(ReqAttribute.Key) ? 
            EntityStats.FindRef(ReqAttribute.Key) : 0.0f;
            
        if (EntityAttributeValue < ReqAttribute.Value)
        {
            return false;
        }
    }

    return true;
}

void UCustomItemAsset::ApplyEffects(TMap<FName, float>& EntityStats) const
{
    if (!bConsumable || UsageEffects.Num() == 0)
    {
        return;
    }

    // Apply instant effects
    for (const FItemUsageEffect& Effect : UsageEffects)
    {
        if (Effect.StatName == NAME_None)
        {
            continue;
        }

        // Instant effect (duration == 0)
        if (Effect.Duration <= 0.0f)
        {
            float CurrentValue = EntityStats.Contains(Effect.StatName) ? EntityStats[Effect.StatName] : 0.0f;
            EntityStats.Add(Effect.StatName, CurrentValue + Effect.Value);
        }
        else
        {
            // For effects with duration, we would need a buff/debuff system
            // This is where you would integrate with your game's status effect system
            // For now, we'll just log that a timed effect would be applied
            UE_LOG(LogTemp, Display, TEXT("Item %s would apply timed effect to %s: %f for %f seconds"), 
                *AssetId.ToString(), *Effect.StatName.ToString(), Effect.Value, Effect.Duration);
        }
    }
}

FLinearColor UCustomItemAsset::GetQualityColor() const
{
    switch (Quality)
    {
        case EItemQuality::Common:
            return FLinearColor(0.7f, 0.7f, 0.7f); // Gray
        case EItemQuality::Uncommon:
            return FLinearColor(0.0f, 0.7f, 0.0f); // Green
        case EItemQuality::Rare:
            return FLinearColor(0.0f, 0.5f, 1.0f); // Blue
        case EItemQuality::Epic:
            return FLinearColor(0.5f, 0.0f, 1.0f); // Purple
        case EItemQuality::Legendary:
            return FLinearColor(1.0f, 0.5f, 0.0f); // Orange
        case EItemQuality::Unique:
            return FLinearColor(1.0f, 0.0f, 0.0f); // Red
        default:
            return FLinearColor::White;
    }
}

FText UCustomItemAsset::GetQualityText() const
{
    switch (Quality)
    {
        case EItemQuality::Common:
            return FText::FromString(TEXT("Common"));
        case EItemQuality::Uncommon:
            return FText::FromString(TEXT("Uncommon"));
        case EItemQuality::Rare:
            return FText::FromString(TEXT("Rare"));
        case EItemQuality::Epic:
            return FText::FromString(TEXT("Epic"));
        case EItemQuality::Legendary:
            return FText::FromString(TEXT("Legendary"));
        case EItemQuality::Unique:
            return FText::FromString(TEXT("Unique"));
        default:
            return FText::FromString(TEXT("Unknown"));
    }
}

bool UCustomItemAsset::MigrateFromVersion(int32 OldVersion)
{
    bool bMigrated = Super::MigrateFromVersion(OldVersion);
    
    // Version migration logic
    if (OldVersion < 2) // Example: In version 2 we added the Quality enum
    {
        // Handle migration logic here
        // For example, if old items had a numeric Rarity value, we could convert to the new enum
        
        // This is a simplified example and would need to be adapted to your actual versioning needs
        bMigrated = true;
    }
    
    return bMigrated;
}

void UCustomItemAsset::PostLoad()
{
    Super::PostLoad();
    
    // Validation logic
    if (bStackable && MaxStackSize <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Item %s is stackable but has invalid MaxStackSize (%d). Setting to 1."), 
            *AssetId.ToString(), MaxStackSize);
        MaxStackSize = 1;
    }
    
    if (bConsumable && UsageEffects.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Item %s is consumable but has no usage effects defined."), 
            *AssetId.ToString());
    }
    
    // Additional validation could be added here
} 