#include "Assets/CustomAssetBlueprintLibrary.h"
#include "Assets/CustomAssetManager.h"
#include "Assets/CustomAssetMemoryTracker.h"
#include "Misc/Paths.h"

UCustomAssetManager* UCustomAssetBlueprintLibrary::GetCustomAssetManager()
{
    return &UCustomAssetManager::Get();
}

UCustomAssetBase* UCustomAssetBlueprintLibrary::LoadAssetById(FName AssetId)
{
    return UCustomAssetManager::Get().LoadAssetById(AssetId);
}

UCustomAssetBase* UCustomAssetBlueprintLibrary::LoadAssetByIdWithStrategy(FName AssetId, EAssetLoadingStrategy Strategy)
{
    return UCustomAssetManager::Get().LoadAssetByIdWithStrategy(AssetId, Strategy);
}

bool UCustomAssetBlueprintLibrary::UnloadAssetById(FName AssetId)
{
    return UCustomAssetManager::Get().UnloadAssetById(AssetId);
}

UCustomAssetBase* UCustomAssetBlueprintLibrary::GetAssetById(FName AssetId)
{
    return UCustomAssetManager::Get().GetAssetById(AssetId);
}

TArray<FName> UCustomAssetBlueprintLibrary::GetAllLoadedAssetIds()
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    
    TArray<UCustomAssetBase*> LoadedAssets = AssetManager.GetAllLoadedAssets();
    TArray<FName> LoadedAssetIds;
    
    for (UCustomAssetBase* Asset : LoadedAssets)
    {
        if (Asset)
        {
            LoadedAssetIds.Add(Asset->AssetId);
        }
    }
    
    return LoadedAssetIds;
}

TArray<FName> UCustomAssetBlueprintLibrary::GetAllAssetIds()
{
    return UCustomAssetManager::Get().GetAllAssetIds();
}

void UCustomAssetBlueprintLibrary::PreloadAssets(const TArray<FName>& AssetIds)
{
    UCustomAssetManager::Get().PreloadAssets(AssetIds);
}

void UCustomAssetBlueprintLibrary::LoadBundle(FName BundleId)
{
    UCustomAssetManager::Get().LoadBundle(BundleId, EAssetLoadingStrategy::OnDemand);
}

void UCustomAssetBlueprintLibrary::LoadBundleWithStrategy(FName BundleId, EAssetLoadingStrategy Strategy)
{
    UCustomAssetManager::Get().LoadBundle(BundleId, Strategy);
}

void UCustomAssetBlueprintLibrary::UnloadBundle(FName BundleId)
{
    UCustomAssetManager::Get().UnloadBundle(BundleId);
}

UCustomAssetBundle* UCustomAssetBlueprintLibrary::GetBundleById(FName BundleId)
{
    return UCustomAssetManager::Get().GetBundleById(BundleId);
}

TArray<FName> UCustomAssetBlueprintLibrary::GetAllBundleIds()
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    
    TArray<UCustomAssetBundle*> Bundles;
    AssetManager.GetAllBundles(Bundles);
    TArray<FName> BundleIds;
    
    for (UCustomAssetBundle* Bundle : Bundles)
    {
        if (Bundle)
        {
            BundleIds.Add(Bundle->BundleId);
        }
    }
    
    return BundleIds;
}

TArray<FName> UCustomAssetBlueprintLibrary::GetAssetsInBundle(FName BundleId)
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    UCustomAssetBundle* Bundle = AssetManager.GetBundleById(BundleId);
    
    if (Bundle)
    {
        return Bundle->AssetIds;
    }
    
    return TArray<FName>();
}

TArray<FName> UCustomAssetBlueprintLibrary::GetAssetDependencies(FName AssetId, bool bHardDependenciesOnly)
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
    
    if (Asset)
    {
        if (bHardDependenciesOnly)
        {
            return Asset->GetHardDependencies();
        }
        else
        {
            TArray<FName> DependencyIds;
            for (const FCustomAssetDependency& Dependency : Asset->Dependencies)
            {
                DependencyIds.Add(Dependency.DependentAssetId);
            }
            return DependencyIds;
        }
    }
    
    return TArray<FName>();
}

TArray<FName> UCustomAssetBlueprintLibrary::GetDependentAssets(FName AssetId, bool bHardDependenciesOnly)
{
    return UCustomAssetManager::Get().GetDependentAssets(AssetId, bHardDependenciesOnly);
}

bool UCustomAssetBlueprintLibrary::ExportDependencyGraph(const FString& FilePath)
{
    FString FullPath = FPaths::ConvertRelativePathToFull(FilePath);
    return UCustomAssetManager::Get().ExportDependencyGraph(FullPath);
}

int32 UCustomAssetBlueprintLibrary::GetCurrentMemoryUsage()
{
    return UCustomAssetManager::Get().GetCurrentMemoryUsage();
}

int32 UCustomAssetBlueprintLibrary::GetMemoryUsageThreshold()
{
    return UCustomAssetManager::Get().GetMemoryUsageThreshold();
}

void UCustomAssetBlueprintLibrary::SetMemoryUsageThreshold(int32 ThresholdMB)
{
    UCustomAssetManager::Get().SetMemoryUsageThreshold(ThresholdMB);
}

void UCustomAssetBlueprintLibrary::SetMemoryManagementPolicy(EMemoryManagementPolicy Policy)
{
    UCustomAssetManager::Get().SetMemoryManagementPolicy(Policy);
}

bool UCustomAssetBlueprintLibrary::ExportMemoryUsageToCSV(const FString& FilePath)
{
    FString FullPath = FPaths::ConvertRelativePathToFull(FilePath);
    return UCustomAssetManager::Get().ExportMemoryUsageToCSV(FullPath);
}

FAssetMemoryStats UCustomAssetBlueprintLibrary::GetAssetMemoryStats(FName AssetId)
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    UCustomAssetMemoryTracker* MemoryTracker = AssetManager.GetMemoryTracker();
    
    if (MemoryTracker)
    {
        return MemoryTracker->GetAssetMemoryStats(AssetId);
    }
    
    return FAssetMemoryStats();
}

bool UCustomAssetBlueprintLibrary::ExportAssetsToCSV(const FString& FilePath)
{
    FString FullPath = FPaths::ConvertRelativePathToFull(FilePath);
    return UCustomAssetManager::Get().ExportAssetsToCSV(FullPath);
}

FText UCustomAssetBlueprintLibrary::GetAssetDisplayName(FName AssetId)
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
    
    if (Asset)
    {
        return Asset->DisplayName;
    }
    
    return FText::FromString(TEXT("Unknown Asset"));
}

FText UCustomAssetBlueprintLibrary::GetAssetDescription(FName AssetId)
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
    
    if (Asset)
    {
        return Asset->Description;
    }
    
    return FText::GetEmpty();
}

TArray<FName> UCustomAssetBlueprintLibrary::GetAssetTags(FName AssetId)
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
    
    if (Asset)
    {
        return Asset->Tags;
    }
    
    return TArray<FName>();
}

int32 UCustomAssetBlueprintLibrary::GetAssetVersion(FName AssetId)
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
    
    if (Asset)
    {
        return Asset->Version;
    }
    
    return 0;
}

TArray<FAssetVersionChange> UCustomAssetBlueprintLibrary::GetAssetVersionHistory(FName AssetId)
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
    
    if (Asset)
    {
        return Asset->VersionHistory;
    }
    
    return TArray<FAssetVersionChange>();
}

// Item Asset Functions

UCustomItemAsset* UCustomAssetBlueprintLibrary::GetItemAsset(FName AssetId, bool bLoadIfNecessary)
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    
    UCustomAssetBase* BaseAsset = bLoadIfNecessary ? 
        AssetManager.LoadAssetById(AssetId) : 
        AssetManager.GetAssetById(AssetId);
    
    return Cast<UCustomItemAsset>(BaseAsset);
}

TArray<FName> UCustomAssetBlueprintLibrary::GetAllItemAssetIds()
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    TArray<FName> AllAssetIds = AssetManager.GetAllAssetIds();
    TArray<FName> ItemAssetIds;
    
    for (const FName& AssetId : AllAssetIds)
    {
        UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
        if (Asset && Asset->IsA<UCustomItemAsset>())
        {
            ItemAssetIds.Add(AssetId);
        }
    }
    
    return ItemAssetIds;
}

TArray<UCustomItemAsset*> UCustomAssetBlueprintLibrary::GetItemAssetsByQuality(EItemQuality Quality, bool bLoadAssets)
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    TArray<FName> AllAssetIds = AssetManager.GetAllAssetIds();
    TArray<UCustomItemAsset*> MatchingItems;
    
    for (const FName& AssetId : AllAssetIds)
    {
        UCustomAssetBase* BaseAsset = bLoadAssets ? 
            AssetManager.LoadAssetById(AssetId) : 
            AssetManager.GetAssetById(AssetId);
        
        UCustomItemAsset* ItemAsset = Cast<UCustomItemAsset>(BaseAsset);
        if (ItemAsset && ItemAsset->Quality == Quality)
        {
            MatchingItems.Add(ItemAsset);
        }
    }
    
    return MatchingItems;
}

TArray<UCustomItemAsset*> UCustomAssetBlueprintLibrary::GetItemAssetsByCategory(FName Category, bool bLoadAssets)
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    TArray<FName> AllAssetIds = AssetManager.GetAllAssetIds();
    TArray<UCustomItemAsset*> MatchingItems;
    
    for (const FName& AssetId : AllAssetIds)
    {
        UCustomAssetBase* BaseAsset = bLoadAssets ? 
            AssetManager.LoadAssetById(AssetId) : 
            AssetManager.GetAssetById(AssetId);
        
        UCustomItemAsset* ItemAsset = Cast<UCustomItemAsset>(BaseAsset);
        if (ItemAsset && ItemAsset->Category == Category)
        {
            MatchingItems.Add(ItemAsset);
        }
    }
    
    return MatchingItems;
}

bool UCustomAssetBlueprintLibrary::ApplyItemEffects(UCustomItemAsset* ItemAsset, TMap<FName, float>& Stats)
{
    if (!ItemAsset)
    {
        return false;
    }
    
    ItemAsset->ApplyEffects(Stats);
    return true;
}

bool UCustomAssetBlueprintLibrary::CanEntityUseItem(UCustomItemAsset* ItemAsset, const TMap<FName, float>& EntityStats)
{
    if (!ItemAsset)
    {
        return false;
    }
    
    return ItemAsset->CanBeUsedBy(EntityStats);
}

// Character Asset Functions

UCustomCharacterAsset* UCustomAssetBlueprintLibrary::GetCharacterAsset(FName AssetId, bool bLoadIfNecessary)
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    
    UCustomAssetBase* BaseAsset = bLoadIfNecessary ? 
        AssetManager.LoadAssetById(AssetId) : 
        AssetManager.GetAssetById(AssetId);
    
    return Cast<UCustomCharacterAsset>(BaseAsset);
}

TArray<FName> UCustomAssetBlueprintLibrary::GetAllCharacterAssetIds()
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    TArray<FName> AllAssetIds = AssetManager.GetAllAssetIds();
    TArray<FName> CharacterAssetIds;
    
    for (const FName& AssetId : AllAssetIds)
    {
        UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
        if (Asset && Asset->IsA<UCustomCharacterAsset>())
        {
            CharacterAssetIds.Add(AssetId);
        }
    }
    
    return CharacterAssetIds;
}

TArray<UCustomCharacterAsset*> UCustomAssetBlueprintLibrary::GetCharacterAssetsByClass(ECharacterClass CharacterClass, bool bLoadAssets)
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    TArray<FName> AllAssetIds = AssetManager.GetAllAssetIds();
    TArray<UCustomCharacterAsset*> MatchingCharacters;
    
    for (const FName& AssetId : AllAssetIds)
    {
        UCustomAssetBase* BaseAsset = bLoadAssets ? 
            AssetManager.LoadAssetById(AssetId) : 
            AssetManager.GetAssetById(AssetId);
        
        UCustomCharacterAsset* CharacterAsset = Cast<UCustomCharacterAsset>(BaseAsset);
        if (CharacterAsset && CharacterAsset->CharacterClass == CharacterClass)
        {
            MatchingCharacters.Add(CharacterAsset);
        }
    }
    
    return MatchingCharacters;
}

TArray<UCustomCharacterAsset*> UCustomAssetBlueprintLibrary::GetCharacterAssetsByLevelRange(int32 MinLevel, int32 MaxLevel, bool bLoadAssets)
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    TArray<FName> AllAssetIds = AssetManager.GetAllAssetIds();
    TArray<UCustomCharacterAsset*> MatchingCharacters;
    
    for (const FName& AssetId : AllAssetIds)
    {
        UCustomAssetBase* BaseAsset = bLoadAssets ? 
            AssetManager.LoadAssetById(AssetId) : 
            AssetManager.GetAssetById(AssetId);
        
        UCustomCharacterAsset* CharacterAsset = Cast<UCustomCharacterAsset>(BaseAsset);
        if (CharacterAsset && CharacterAsset->Level >= MinLevel && CharacterAsset->Level <= MaxLevel)
        {
            MatchingCharacters.Add(CharacterAsset);
        }
    }
    
    return MatchingCharacters;
}

int32 UCustomAssetBlueprintLibrary::GetCharacterExperienceForLevel(UCustomCharacterAsset* CharacterAsset, int32 TargetLevel)
{
    if (!CharacterAsset)
    {
        return 0;
    }
    
    return CharacterAsset->GetExperienceForLevel(TargetLevel);
}

TArray<FName> UCustomAssetBlueprintLibrary::GetCharacterAbilities(UCustomCharacterAsset* CharacterAsset)
{
    if (!CharacterAsset)
    {
        return TArray<FName>();
    }
    
    return CharacterAsset->GetAbilityIds();
}

FCharacterAbility UCustomAssetBlueprintLibrary::GetCharacterAbility(UCustomCharacterAsset* CharacterAsset, FName AbilityId, bool& bFound)
{
    bFound = false;
    
    if (!CharacterAsset)
    {
        return FCharacterAbility();
    }
    
    bFound = CharacterAsset->HasAbility(AbilityId);
    return CharacterAsset->GetAbility(AbilityId);
}

TArray<FName> UCustomAssetBlueprintLibrary::GetAbilitiesUnlockedAtLevel(UCustomCharacterAsset* CharacterAsset, int32 Level)
{
    TArray<FName> UnlockedAbilities;
    
    if (!CharacterAsset)
    {
        return UnlockedAbilities;
    }
    
    // Get all abilities unlocked at the exact level
    for (const FCharacterAbility& Ability : CharacterAsset->Abilities)
    {
        if (Ability.RequiredLevel == Level)
        {
            UnlockedAbilities.Add(Ability.AbilityId);
        }
    }
    
    return UnlockedAbilities;
}

// LOD Functions

UStaticMesh* UCustomAssetBlueprintLibrary::GetItemLODMesh(UCustomItemAsset* ItemAsset, float Distance)
{
    if (!ItemAsset)
    {
        return nullptr;
    }
    
    if (ItemAsset->bUseLOD && Distance >= ItemAsset->LODSwitchDistance && ItemAsset->LowDetailMesh.LoadSynchronous())
    {
        return ItemAsset->LowDetailMesh.Get();
    }
    
    return ItemAsset->ItemMesh.LoadSynchronous();
}

USkeletalMesh* UCustomAssetBlueprintLibrary::GetCharacterLODMesh(UCustomCharacterAsset* CharacterAsset, float Distance)
{
    if (!CharacterAsset)
    {
        return nullptr;
    }
    
    if (CharacterAsset->bUseLOD && Distance >= CharacterAsset->LODSwitchDistance && CharacterAsset->LowDetailMesh.LoadSynchronous())
    {
        return CharacterAsset->LowDetailMesh.Get();
    }
    
    return CharacterAsset->CharacterMesh.LoadSynchronous();
}

// Asset Tags Functions

TArray<UCustomAssetBase*> UCustomAssetBlueprintLibrary::GetAssetsByTag(FName Tag, bool bLoadAssets)
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    TArray<FName> AllAssetIds = AssetManager.GetAllAssetIds();
    TArray<UCustomAssetBase*> MatchingAssets;
    
    for (const FName& AssetId : AllAssetIds)
    {
        UCustomAssetBase* BaseAsset = bLoadAssets ? 
            AssetManager.LoadAssetById(AssetId) : 
            AssetManager.GetAssetById(AssetId);
        
        if (BaseAsset && BaseAsset->Tags.Contains(Tag))
        {
            MatchingAssets.Add(BaseAsset);
        }
    }
    
    return MatchingAssets;
}

bool UCustomAssetBlueprintLibrary::DoesAssetHaveTag(FName AssetId, FName Tag)
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
    
    if (Asset)
    {
        return Asset->Tags.Contains(Tag);
    }
    
    return false;
}

bool UCustomAssetBlueprintLibrary::AddTagToAsset(FName AssetId, FName Tag)
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
    
    if (Asset)
    {
        if (!Asset->Tags.Contains(Tag))
        {
            Asset->Tags.Add(Tag);
            return true;
        }
    }
    
    return false;
}

bool UCustomAssetBlueprintLibrary::RemoveTagFromAsset(FName AssetId, FName Tag)
{
    UCustomAssetManager& AssetManager = UCustomAssetManager::Get();
    UCustomAssetBase* Asset = AssetManager.GetAssetById(AssetId);
    
    if (Asset)
    {
        return Asset->Tags.Remove(Tag) > 0;
    }
    
    return false;
} 