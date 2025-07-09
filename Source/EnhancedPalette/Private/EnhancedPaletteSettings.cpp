#include "EnhancedPaletteSettings.h"

#include "ActorFactories/ActorFactoryClass.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "IPlacementModeModule.h"
#include "Styling/SlateStyle.h"
#include "Textures/SlateIcon.h"
#include "PrivateAccessHelper.h"
#include "Subsystems/PlacementSubsystem.h"
#include "EnhancedPaletteCustomizations.h"

/*
 *
FPlaceableItem(TScriptInterface<IAssetFactoryInterface> InAssetFactory,
		 const FAssetData& InAssetData,
		 FName InClassThumbnailBrushOverride,
		 FName InClassIconBrushOverride,
		 TOptional<FLinearColor> InAssetTypeColorOverride = TOptional<FLinearColor>(),
		 TOptional<int32> InSortOrder,
		 TOptional<FText> InDisplayName = TOptional<FText>())

FPlaceableItem(UClass& InActorFactoryClass,
		const FAssetData& InAssetData,
		FName InClassThumbnailBrushOverride = NAME_None,
		FName InClassIconBrushOverride = NAME_None,
		TOptional<FLinearColor> InAssetTypeColorOverride = TOptional<FLinearColor>(),
		TOptional<int32> InSortOrder = TOptional<int32>(),
		TOptional<FText> InDisplayName = TOptional<FText>()
	 )

	 @see FPlaceableItem::FPlaceableItem
 *
 */

inline TOptional<FText> MakeOpionalDesc(const FConfigPlaceableItem* Self)
{
	return Self->DisplayName.IsEmptyOrWhitespace() ? TOptional<FText>() : Self->DisplayName;
}
inline TOptional<int32> MakeOpionalOrder(const FConfigPlaceableItem* Self)
{
	return Self->SortOrder ? TOptional<int32>() : Self->SortOrder;
}

template<typename TPtr>
bool TryRequestActorFactory(const TSoftClassPtr<TPtr>& TypePtr, UActorFactory*& FactoryPtr)
{
	FactoryPtr = nullptr;
	if (const UClass* LoadedClass = TypePtr.Get())
	{
		FactoryPtr = GEditor->FindActorFactoryByClass(LoadedClass);
	}
	else if (const UClass* SyncLoaded = TypePtr.LoadSynchronous())
	{
		FactoryPtr = GEditor->FindActorFactoryByClass(SyncLoaded);
	}
	return FactoryPtr != nullptr;
}

template<typename TPtr>
bool TryRequestAssetData(const TPtr& ObjectPtr, FAssetData& OutAssetData)
{
	if (ObjectPtr.IsValid())
	{
		OutAssetData = FAssetData(ObjectPtr.Get());
		return true;
	}

	FSoftObjectPath Path = ObjectPtr.ToSoftObjectPath();
	if (IAssetRegistry::GetChecked().TryGetAssetByObjectPath(Path, OutAssetData) == UE::AssetRegistry::EExists::Exists)
	{
		return true;
	}

	return false;
}

TSharedPtr<FPlaceableItem> FConfigPlaceableItem::MakeItem() const
{
	checkNoEntry();
	return nullptr;
}

FString FConfigPlaceableItem::ToString() const
{
	return DisplayName.ToString();
}

bool FConfigPlaceableItem::IsValidData() const
{
	checkNoEntry();
	return false;
}

FConfigPlaceableItem_Native::FConfigPlaceableItem_Native(TSharedPtr<FPlaceableItem> InItem) : Item(MoveTempIfPossible(InItem))
{
}

bool FConfigPlaceableItem_Native::IsValidData() const
{
	return Item.IsValid();
}

TSharedPtr<FPlaceableItem> FConfigPlaceableItem_Native::MakeItem() const
{
	return Item;
}

FConfigPlaceableItem_FactoryClass::FConfigPlaceableItem_FactoryClass(TSoftClassPtr<UObject> InClass)
	: FactoryClass(InClass)
{
	
}

bool FConfigPlaceableItem_FactoryClass::IsValidData() const
{
	return !FactoryClass.IsNull();
}

TSharedPtr<FPlaceableItem> FConfigPlaceableItem_FactoryClass::MakeItem() const
{
	UActorFactory* Factory = GEditor->FindActorFactoryByClass(FactoryClass.LoadSynchronous());
	if (!TryRequestActorFactory(FactoryClass, Factory))
	{
		return nullptr;
	}

	FAssetData AssetData = Factory ? Factory->GetDefaultActorClass(FAssetData()) : FAssetData();
	
	return MakeShared<FPlaceableItem>(
		Factory,
		AssetData,
		NAME_None, NAME_None, TOptional<FLinearColor>(), // brush customization
		MakeOpionalOrder(this), MakeOpionalDesc(this));
}

FConfigPlaceableItem_FactoryAsset::FConfigPlaceableItem_FactoryAsset(TSoftClassPtr<UObject> InFactoryClass, const FAssetData& InAssetData)
	: FactoryClass(InFactoryClass), ObjectData(InAssetData)
{
	
}

bool FConfigPlaceableItem_FactoryAsset::IsValidData() const
{
	return !FactoryClass.IsNull() && ObjectData.IsValid();
}

inline TSharedPtr<FPlaceableItem> FConfigPlaceableItem_FactoryAsset::MakeItem() const
{
	UActorFactory* Factory = GEditor->FindActorFactoryByClass(FactoryClass.LoadSynchronous());
	if (!Factory)
	{
		return nullptr;
	}

	if (!ObjectData.IsValid())
	{
		return nullptr;
	}

	return MakeShared<FPlaceableItem>(
		Factory,
		ObjectData,
		NAME_None, NAME_None, TOptional<FLinearColor>(), // brush customization
		MakeOpionalOrder(this), MakeOpionalDesc(this));
}

FConfigPlaceableItem_FactoryObject::FConfigPlaceableItem_FactoryObject(TSoftClassPtr<UObject> InFactoryClass, TSoftObjectPtr<UObject> InObject)
	: FactoryClass(InFactoryClass), Object(InObject)
{
	
}

bool FConfigPlaceableItem_FactoryObject::IsValidData() const
{
	return !FactoryClass.IsNull() && !Object.IsNull();
}

TSharedPtr<FPlaceableItem> FConfigPlaceableItem_FactoryObject::MakeItem() const
{
	UActorFactory* Factory = GEditor->FindActorFactoryByClass(FactoryClass.LoadSynchronous());

	FAssetData AssetData = IAssetRegistry::Get()->GetAssetByObjectPath(Object.ToSoftObjectPath());
	
	return MakeShared<FPlaceableItem>(
		Factory,
		AssetData,
		NAME_None, NAME_None, TOptional<FLinearColor>(), // brush customization
		MakeOpionalOrder(this), MakeOpionalDesc(this));
}

FConfigPlaceableItem_ActorClass::FConfigPlaceableItem_ActorClass(TSoftClassPtr<AActor> InClass)
	: ActorClass(InClass)
{
	
}

bool FConfigPlaceableItem_ActorClass::IsValidData() const
{
	return !ActorClass.IsNull();
}

TSharedPtr<FPlaceableItem> FConfigPlaceableItem_ActorClass::MakeItem() const
{
	FAssetData AssetData;
	if (ActorClass.IsValid())
	{
		AssetData = FAssetData(ActorClass.Get());
	}
	else if (IAssetRegistry::GetChecked().TryGetAssetByObjectPath(ActorClass.ToSoftObjectPath(), AssetData) != UE::AssetRegistry::EExists::Exists)
	{
		return nullptr;
	}

	if (!AssetData.IsValid())
	{
		return nullptr;
	}

	TScriptInterface<IAssetFactoryInterface> ActorFactory = nullptr;
	if (UPlacementSubsystem* PlacementSubsystem = GEditor->GetEditorSubsystem<UPlacementSubsystem>())
	{
		ActorFactory = PlacementSubsystem->FindAssetFactoryFromAssetData(AssetData);
	}

	if (!ActorFactory)
	{
		ActorFactory = GEditor->FindActorFactoryByClass(UActorFactoryClass::StaticClass());
	}

	return MakeShared<FPlaceableItem>(
		ActorFactory,
		AssetData,
		NAME_None, NAME_None, TOptional<FLinearColor>(),
		MakeOpionalOrder(this), MakeOpionalDesc(this));
}

FConfigPlaceableItem_AssetObject::FConfigPlaceableItem_AssetObject(TSoftObjectPtr<UObject> InObject)
	: Object(InObject)
{
}

bool FConfigPlaceableItem_AssetObject::IsValidData() const
{
	return !Object.IsNull();
}

TSharedPtr<FPlaceableItem> FConfigPlaceableItem_AssetObject::MakeItem() const
{
	FAssetData AssetData;
	if (Object.IsValid())
	{
		AssetData = FAssetData(Object.Get());
	}
	else
	{
		AssetData = IAssetRegistry::GetChecked().GetAssetByObjectPath(Object.ToSoftObjectPath());
	}

	if (!AssetData.IsValid())
	{
		return nullptr;
	}

	TScriptInterface<IAssetFactoryInterface> ActorFactory = nullptr;
	if (UPlacementSubsystem* PlacementSubsystem = GEditor->GetEditorSubsystem<UPlacementSubsystem>())
	{
		ActorFactory = PlacementSubsystem->FindAssetFactoryFromAssetData(AssetData);
	}

	if (!ActorFactory)
	{
		return nullptr;
	}

	return MakeShared<FPlaceableItem>(
		ActorFactory,
		AssetData,
		NAME_None, NAME_None, TOptional<FLinearColor>(),
		MakeOpionalOrder(this), MakeOpionalDesc(this));
}

bool FStandardPlacementCategoryInfo::CanEditChange(const FEditPropertyChain& PropertyChain) const
{
	static const FName PROP_SortOrder = GET_MEMBER_NAME_CHECKED(FStandardPlacementCategoryInfo, Order);
	
	const FName PropertyName = PropertyChain.GetActiveNode() ? PropertyChain.GetActiveNode()->GetValue()->GetFName() : NAME_None;
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 5, 0)
	if (UniqueId == FBuiltInPlacementCategories::Favorites() && PropertyName == PROP_SortOrder)
		return false;
#endif
	if (UniqueId == FBuiltInPlacementCategories::RecentlyPlaced() && PropertyName == PROP_SortOrder)
		return false;
	
	return true;
}

UEnhancedPaletteSettings::UEnhancedPaletteSettings()
{
#ifdef OR_CAN_I
	ClearFlags(RF_ArchetypeObject);
#endif
}

void UEnhancedPaletteSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UEnhancedPaletteSettings::TriggerUpdateData()
{
	OnUserAction.ExecuteIfBound(SettingsCommand::UpdateCategores);
}

void UEnhancedPaletteSettings::RefreshToolbar()
{
	OnUserAction.ExecuteIfBound(SettingsCommand::UpdateToolbar);
}

void UEnhancedPaletteSettings::ClearRecentlyPlaced()
{
	OnUserAction.ExecuteIfBound(SettingsCommand::ClearRecent);
}

void UEnhancedPaletteSettings::TriggerPopulateItems()
{
	OnUserAction.ExecuteIfBound(SettingsCommand::PopulateCategories);
}

void UEnhancedPaletteSettings::TriggerDiscover()
{
	OnUserAction.ExecuteIfBound(SettingsCommand::DiscoverCategories);
}

