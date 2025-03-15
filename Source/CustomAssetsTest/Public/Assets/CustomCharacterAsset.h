#pragma once

#include "CoreMinimal.h"
#include "Assets/CustomAssetBase.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimMontage.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "CustomCharacterAsset.generated.h"

/**
 * Enum for character class types
 */
UENUM(BlueprintType)
enum class ECharacterClass : uint8
{
    Warrior     UMETA(DisplayName = "Warrior"),
    Ranger      UMETA(DisplayName = "Ranger"),
    Mage        UMETA(DisplayName = "Mage"),
    Rogue       UMETA(DisplayName = "Rogue"),
    Support     UMETA(DisplayName = "Support"),
    Monster     UMETA(DisplayName = "Monster"),
    NPC         UMETA(DisplayName = "NPC")
};

/**
 * Struct for character ability
 */
USTRUCT(BlueprintType)
struct CUSTOMASSETSTEST_API FCharacterAbility
{
    GENERATED_BODY()

    // Ability identifier
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
    FName AbilityId;

    // Display name
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
    FText DisplayName;

    // Animation montage for the ability
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
    TSoftObjectPtr<UAnimMontage> AbilityMontage;

    // Particle effect for the ability
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
    TSoftObjectPtr<UParticleSystem> AbilityEffect;

    // Sound effect for the ability
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
    TSoftObjectPtr<USoundCue> AbilitySound;

    // Cooldown time in seconds
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", meta = (ClampMin = "0.0"))
    float Cooldown;

    // Ability cost (e.g., mana, energy)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
    float Cost;

    // Required level to unlock
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", meta = (ClampMin = "1"))
    int32 RequiredLevel;

    // Default constructor
    FCharacterAbility()
        : AbilityId(NAME_None)
        , DisplayName(FText::FromString("New Ability"))
        , Cooldown(1.0f)
        , Cost(0.0f)
        , RequiredLevel(1)
    {
    }
};

/**
 * Custom asset type for character data
 */
UCLASS(BlueprintType)
class CUSTOMASSETSTEST_API UCustomCharacterAsset : public UCustomAssetBase
{
    GENERATED_BODY()

public:
    UCustomCharacterAsset();

    // Character mesh
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Visuals")
    TSoftObjectPtr<USkeletalMesh> CharacterMesh;

    // Character animation blueprint
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Visuals")
    TSoftObjectPtr<UAnimBlueprint> AnimBlueprint;

    // Physics asset
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Visuals")
    TSoftObjectPtr<UPhysicsAsset> PhysicsAsset;

    // Character portrait for UI
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Visuals")
    TSoftObjectPtr<UTexture2D> Portrait;

    // Character class
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Properties")
    ECharacterClass CharacterClass;

    // Base health value
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Stats", meta = (ClampMin = "1.0"))
    float BaseHealth;

    // Base movement speed
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Stats", meta = (ClampMin = "1.0"))
    float BaseMovementSpeed;

    // Base attributes
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Stats")
    TMap<FName, float> BaseAttributes;

    // Character abilities
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Abilities")
    TArray<FCharacterAbility> Abilities;

    // Character level (starting level)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Progression", meta = (ClampMin = "1"))
    int32 Level;

    // Experience required for first level up
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Progression", meta = (ClampMin = "1"))
    int32 BaseExperienceRequired;

    // Experience scaling factor per level
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Progression", meta = (ClampMin = "1.0"))
    float ExperienceScaling;

    // LOD settings
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character LOD")
    bool bUseLOD;

    // Lower detail mesh for LOD
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character LOD", meta = (EditCondition = "bUseLOD"))
    TSoftObjectPtr<USkeletalMesh> LowDetailMesh;

    // Distance at which to switch to low detail
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character LOD", meta = (EditCondition = "bUseLOD", ClampMin = "0.0"))
    float LODSwitchDistance;

    // Get list of ability IDs
    UFUNCTION(BlueprintCallable, Category = "Character Abilities")
    TArray<FName> GetAbilityIds() const;

    // Get ability by ID
    UFUNCTION(BlueprintCallable, Category = "Character Abilities")
    FCharacterAbility GetAbility(const FName& AbilityId) const;

    // Check if character has a specific ability
    UFUNCTION(BlueprintCallable, Category = "Character Abilities")
    bool HasAbility(const FName& AbilityId) const;

    // Calculate the XP required for a specific level
    UFUNCTION(BlueprintCallable, Category = "Character Progression")
    int32 GetExperienceForLevel(int32 TargetLevel) const;

    // Get all attributes with their base values
    UFUNCTION(BlueprintCallable, Category = "Character Stats")
    TMap<FName, float> GetBaseAttributes() const;

    // Get the color associated with the character class
    UFUNCTION(BlueprintCallable, Category = "Character Properties")
    FLinearColor GetClassColor() const;

    // Override migrate function to handle version changes
    virtual bool MigrateFromVersion(int32 OldVersion) override;

protected:
    // Validate the character configuration
    virtual void PostLoad() override;
}; 