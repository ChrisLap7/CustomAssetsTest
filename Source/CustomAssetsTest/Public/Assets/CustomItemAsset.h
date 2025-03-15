#pragma once

#include "CoreMinimal.h"
#include "Assets/CustomAssetBase.h"
#include "Engine/Texture2D.h"
#include "Engine/DataTable.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "CustomItemAsset.generated.h"

/**
 * Enum for item quality levels
 */
UENUM(BlueprintType)
enum class EItemQuality : uint8
{
    Common      UMETA(DisplayName = "Common"),
    Uncommon    UMETA(DisplayName = "Uncommon"),
    Rare        UMETA(DisplayName = "Rare"),
    Epic        UMETA(DisplayName = "Epic"),
    Legendary   UMETA(DisplayName = "Legendary"),
    Unique      UMETA(DisplayName = "Unique")
};

/**
 * Struct for item usage effect
 */
USTRUCT(BlueprintType)
struct CUSTOMASSETSTEST_API FItemUsageEffect
{
    GENERATED_BODY()

    // Stat to affect
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    FName StatName;

    // Value to apply (can be positive or negative)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    float Value;

    // Duration of the effect in seconds (0 for instant)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    float Duration;

    // Default constructor
    FItemUsageEffect()
        : StatName(NAME_None)
        , Value(0.0f)
        , Duration(0.0f)
    {
    }
};

/**
 * Sample custom asset type for items with enhanced functionality
 */
UCLASS(BlueprintType)
class CUSTOMASSETSTEST_API UCustomItemAsset : public UCustomAssetBase
{
    GENERATED_BODY()

public:
    UCustomItemAsset();

    // Item icon
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Visuals")
    TSoftObjectPtr<UTexture2D> Icon;

    // 3D model for the item (if applicable)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Visuals")
    TSoftObjectPtr<UStaticMesh> ItemMesh;

    // VFX when using the item
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Visuals")
    TSoftObjectPtr<UParticleSystem> UseEffect;

    // Sound when using the item
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Visuals")
    TSoftObjectPtr<USoundCue> UseSound;

    // Economic value
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (ClampMin = "0"))
    int32 Value;

    // Item weight in kilograms
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (ClampMin = "0.0"))
    float Weight;

    // Item quality/rarity
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties")
    EItemQuality Quality;

    // Item category
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties")
    FName Category;

    // Item stackable flag
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties")
    bool bStackable;

    // Maximum stack size (if stackable)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (EditCondition = "bStackable", ClampMin = "1", ClampMax = "999"))
    int32 MaxStackSize;

    // Is the item consumable
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties")
    bool bConsumable;

    // Required level to use
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Requirements", meta = (ClampMin = "1"))
    int32 RequiredLevel;

    // Required attributes or skills to use
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Requirements")
    TMap<FName, int32> RequiredAttributes;

    // Effects when the item is used
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Usage", meta = (EditCondition = "bConsumable"))
    TArray<FItemUsageEffect> UsageEffects;

    // Cooldown time between uses (in seconds)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Usage", meta = (ClampMin = "0.0"))
    float Cooldown;

    // Data table for item stats (allows for flexible configuration)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
    TSoftObjectPtr<UDataTable> ItemStatsTable;

    // LOD settings for different quality levels
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item LOD")
    bool bUseLOD;

    // Lower quality mesh for LOD
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item LOD", meta = (EditCondition = "bUseLOD"))
    TSoftObjectPtr<UStaticMesh> LowDetailMesh;

    // Distance at which to switch to low detail
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item LOD", meta = (EditCondition = "bUseLOD", ClampMin = "0.0"))
    float LODSwitchDistance;

    // Validate the item can be used by an entity with the given stats
    UFUNCTION(BlueprintCallable, Category = "Item Usage")
    bool CanBeUsedBy(const TMap<FName, float>& EntityStats) const;

    // Apply the item effects to the given stats map
    UFUNCTION(BlueprintCallable, Category = "Item Usage")
    void ApplyEffects(UPARAM(ref) TMap<FName, float>& EntityStats) const;

    // Get the color associated with the item quality
    UFUNCTION(BlueprintCallable, Category = "Item Properties")
    FLinearColor GetQualityColor() const;

    // Get the text description of the item quality
    UFUNCTION(BlueprintCallable, Category = "Item Properties")
    FText GetQualityText() const;

    // Override migrate function to handle version changes
    virtual bool MigrateFromVersion(int32 OldVersion) override;

protected:
    // Validate the item configuration
    virtual void PostLoad() override;
}; 