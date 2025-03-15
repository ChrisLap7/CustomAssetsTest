#include "Assets/CustomCharacterAsset.h"

UCustomCharacterAsset::UCustomCharacterAsset()
{
    // Initialize default values
    CharacterClass = ECharacterClass::Warrior;
    BaseHealth = 100.0f;
    BaseMovementSpeed = 600.0f;
    Level = 1;
    BaseExperienceRequired = 1000;
    ExperienceScaling = 1.5f;
    bUseLOD = false;
    LODSwitchDistance = 2000.0f;
    
    // Add some default attributes
    BaseAttributes.Add(TEXT("Strength"), 10.0f);
    BaseAttributes.Add(TEXT("Dexterity"), 10.0f);
    BaseAttributes.Add(TEXT("Intelligence"), 10.0f);
    BaseAttributes.Add(TEXT("Constitution"), 10.0f);
}

TArray<FName> UCustomCharacterAsset::GetAbilityIds() const
{
    TArray<FName> AbilityIds;
    
    for (const FCharacterAbility& Ability : Abilities)
    {
        if (Ability.AbilityId != NAME_None)
        {
            AbilityIds.Add(Ability.AbilityId);
        }
    }
    
    return AbilityIds;
}

FCharacterAbility UCustomCharacterAsset::GetAbility(const FName& AbilityId) const
{
    for (const FCharacterAbility& Ability : Abilities)
    {
        if (Ability.AbilityId == AbilityId)
        {
            return Ability;
        }
    }
    
    // Return an empty ability if not found
    return FCharacterAbility();
}

bool UCustomCharacterAsset::HasAbility(const FName& AbilityId) const
{
    for (const FCharacterAbility& Ability : Abilities)
    {
        if (Ability.AbilityId == AbilityId)
        {
            return true;
        }
    }
    
    return false;
}

int32 UCustomCharacterAsset::GetExperienceForLevel(int32 TargetLevel) const
{
    if (TargetLevel <= 1)
    {
        return 0; // Level 1 requires no XP
    }
    
    // Formula: BaseXP * (ExperienceScaling)^(TargetLevel-2)
    // This creates an exponential curve for XP requirements
    
    float XPRequired = BaseExperienceRequired;
    for (int32 CurrentLevel = 2; CurrentLevel < TargetLevel; ++CurrentLevel)
    {
        XPRequired = FMath::FloorToInt(XPRequired * ExperienceScaling);
    }
    
    return FMath::FloorToInt(XPRequired);
}

TMap<FName, float> UCustomCharacterAsset::GetBaseAttributes() const
{
    return BaseAttributes;
}

FLinearColor UCustomCharacterAsset::GetClassColor() const
{
    switch (CharacterClass)
    {
        case ECharacterClass::Warrior:
            return FLinearColor(0.8f, 0.0f, 0.0f); // Red
        case ECharacterClass::Ranger:
            return FLinearColor(0.0f, 0.8f, 0.0f); // Green
        case ECharacterClass::Mage:
            return FLinearColor(0.0f, 0.0f, 0.8f); // Blue
        case ECharacterClass::Rogue:
            return FLinearColor(0.8f, 0.8f, 0.0f); // Yellow
        case ECharacterClass::Support:
            return FLinearColor(0.0f, 0.8f, 0.8f); // Cyan
        case ECharacterClass::Monster:
            return FLinearColor(0.5f, 0.0f, 0.5f); // Purple
        case ECharacterClass::NPC:
            return FLinearColor(0.5f, 0.5f, 0.5f); // Gray
        default:
            return FLinearColor::White;
    }
}

bool UCustomCharacterAsset::MigrateFromVersion(int32 OldVersion)
{
    bool bMigrated = Super::MigrateFromVersion(OldVersion);
    
    // Version migration logic
    if (OldVersion < 2) // Example: In version 2 we added the CharacterClass enum
    {
        // Handle migration logic here
        // This is a simplified example and would need to be adapted to your actual versioning needs
        bMigrated = true;
    }
    
    return bMigrated;
}

void UCustomCharacterAsset::PostLoad()
{
    Super::PostLoad();
    
    // Validation logic
    
    // Check if we have a character mesh
    if (!CharacterMesh.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Character %s has no character mesh assigned."), 
            *AssetId.ToString());
    }
    
    // Check if ability IDs are unique
    TSet<FName> UniqueAbilityIds;
    for (const FCharacterAbility& Ability : Abilities)
    {
        if (Ability.AbilityId != NAME_None)
        {
            if (UniqueAbilityIds.Contains(Ability.AbilityId))
            {
                UE_LOG(LogTemp, Warning, TEXT("Character %s has duplicate ability ID: %s"), 
                    *AssetId.ToString(), *Ability.AbilityId.ToString());
            }
            else
            {
                UniqueAbilityIds.Add(Ability.AbilityId);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Character %s has an ability with no ID assigned"), 
                *AssetId.ToString());
        }
    }
    
    // Check if we have any abilities
    if (Abilities.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Character %s has no abilities defined"), 
            *AssetId.ToString());
    }
    
    // Additional validation could be added here
} 