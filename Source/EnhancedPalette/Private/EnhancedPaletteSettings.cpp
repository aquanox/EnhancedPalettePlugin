#include "EnhancedPaletteSettings.h"

#include "AssetRegistry/IAssetRegistry.h"
#include "IPlacementModeModule.h"
#include "PrivateAccessHelper.h"
#include "Subsystems/PlacementSubsystem.h"

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

/**
 * Helper to assign common optional overrides for FPlaceableItem
 * @param Self 
 * @param TargetItem 
 */
void ConfigurePlaceableItem(const FConfigPlaceableItem* Self, const TSharedPtr<FPlaceableItem>& TargetItem)
{
	if (!Self->NativeName.IsNone())
	{
		TargetItem->NativeName = Self->NativeName.ToString();
	}
	if (!Self->DisplayName.IsEmptyOrWhitespace())
	{
		TargetItem->DisplayName = Self->DisplayName;
	}
	if (Self->SortOrder != 0)
	{
		TargetItem->SortOrder = Self->SortOrder;
	}
}

/**
 *  Helper to query asset factory for specified asset data
 */
template<typename T>
bool TryRequestAssetFactory(const TSoftClassPtr<T>& FactoryClassPtr, TScriptInterface<IAssetFactoryInterface>& OutFactory)
{
	OutFactory = nullptr;
	if (const UClass* LoadedClass = FactoryClassPtr.Get())
	{
		OutFactory = GEditor->FindActorFactoryByClass(LoadedClass);
	}
	else if (const UClass* SyncLoaded = FactoryClassPtr.LoadSynchronous())
	{
		OutFactory = GEditor->FindActorFactoryByClass(SyncLoaded);
	}
	return OutFactory != nullptr;
}

/**
 *  Helper to query asset factory for specified asset data
 */
bool TryRequestAssetFactoryForAsset(const FAssetData& AssetData, TScriptInterface<IAssetFactoryInterface>& OutFactory)
{
	OutFactory = nullptr;
	if (UPlacementSubsystem* Subsystem = GEditor->GetEditorSubsystem<UPlacementSubsystem>())
	{
		OutFactory = Subsystem->FindAssetFactoryFromAssetData(AssetData);
	}
	return OutFactory != nullptr;
}

/**
 *  Helper to query asset data for soft object ptr without loading it
 */
template<typename T>
bool TryRequestAssetData(const T& ObjectPtr, FAssetData& OutAssetData)
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

// ======================================================================
// ======================================================================

TSharedPtr<FPlaceableItem> FConfigPlaceableItem::MakeItem() const
{
	checkNoEntry();
	return nullptr;
}

FString FConfigPlaceableItem::ToString() const
{
	return DisplayName.ToString();
}

bool FConfigPlaceableItem::IdenticalTo(const FConfigPlaceableItem& Other) const
{
	return false;
}

bool FConfigPlaceableItem::IsValidData() const
{
	checkNoEntry();
	return false;
}

// ======================================================================
// ======================================================================

FConfigPlaceableItem_Native::FConfigPlaceableItem_Native(TSharedPtr<FPlaceableItem> InItem) : Item(MoveTempIfPossible(InItem))
{
}

bool FConfigPlaceableItem_Native::IdenticalTo(const FConfigPlaceableItem& Other) const
{
	//const FConfigPlaceableItem_Native& LocalOther = static_cast<const FConfigPlaceableItem_Native&>(Other);
	//if (Item && LocalOther.Item)
	//{
	//	return Item->AssetFactory == LocalOther.Item->AssetFactory
	//		&& Item->AssetData == LocalOther.Item->AssetData
	//		&& Item->NativeName == LocalOther.Item->NativeName;
	//}
	return false;
}

bool FConfigPlaceableItem_Native::IsValidData() const
{
	return Item.IsValid();
}

TSharedPtr<FPlaceableItem> FConfigPlaceableItem_Native::MakeItem() const
{
	return Item;
}

// ======================================================================
// ======================================================================

FConfigPlaceableItem_FactoryClass::FConfigPlaceableItem_FactoryClass(TSoftClassPtr<UObject> InClass)
	: FactoryClass(InClass)
{
}

bool FConfigPlaceableItem_FactoryClass::IdenticalTo(const FConfigPlaceableItem& Other) const
{
	const FConfigPlaceableItem_FactoryClass& LocalOther = static_cast<const FConfigPlaceableItem_FactoryClass&>(Other);
	return FactoryClass == LocalOther.FactoryClass;
}

bool FConfigPlaceableItem_FactoryClass::IsValidData() const
{
	return !FactoryClass.IsNull();
}

TSharedPtr<FPlaceableItem> FConfigPlaceableItem_FactoryClass::MakeItem() const
{
	if (UClass* LoadedClass = FactoryClass.LoadSynchronous())
	{
		auto Item = MakeShared<FPlaceableItem>(*LoadedClass);
		ConfigurePlaceableItem(this, Item);
		return Item;
	}
	return nullptr;
}

// ======================================================================
// ======================================================================

FConfigPlaceableItem_FactoryAssetData::FConfigPlaceableItem_FactoryAssetData(TSoftClassPtr<UObject> InFactoryClass, const FAssetData& InAssetData)
	: FactoryClass(InFactoryClass), AssetData(InAssetData)
{
}

bool FConfigPlaceableItem_FactoryAssetData::IdenticalTo(const FConfigPlaceableItem& Other) const
{
	const FConfigPlaceableItem_FactoryAssetData& LocalOther = static_cast<const FConfigPlaceableItem_FactoryAssetData&>(Other);
	return FactoryClass == LocalOther.FactoryClass && AssetData == LocalOther.AssetData;
}

bool FConfigPlaceableItem_FactoryAssetData::IsValidData() const
{
	return !FactoryClass.IsNull() && AssetData.IsValid();
}

inline TSharedPtr<FPlaceableItem> FConfigPlaceableItem_FactoryAssetData::MakeItem() const
{
	if (UClass* LoadedClass = FactoryClass.LoadSynchronous())
	{
		auto Item =  MakeShared<FPlaceableItem>(*LoadedClass, AssetData, NAME_None, NAME_None, TOptional<FLinearColor>(), TOptional<int32>(), TOptional<FText>());
		ConfigurePlaceableItem(this, Item);
		return Item;
	}
	return nullptr;
}

// ======================================================================
// ======================================================================

FConfigPlaceableItem_FactoryObject::FConfigPlaceableItem_FactoryObject(TSoftClassPtr<UObject> InFactoryClass, TSoftObjectPtr<UObject> InObject)
	: FactoryClass(InFactoryClass), Object(InObject)
{
	
}

bool FConfigPlaceableItem_FactoryObject::IdenticalTo(const FConfigPlaceableItem& Other) const
{
	const FConfigPlaceableItem_FactoryObject& LocalOther = static_cast<const FConfigPlaceableItem_FactoryObject&>(Other);
	return FactoryClass == LocalOther.FactoryClass && Object == LocalOther.Object;
}

bool FConfigPlaceableItem_FactoryObject::IsValidData() const
{
	return !FactoryClass.IsNull() && !Object.IsNull();
}

TSharedPtr<FPlaceableItem> FConfigPlaceableItem_FactoryObject::MakeItem() const
{
	FAssetData AssetData;
	if (!TryRequestAssetData(Object, AssetData))
	{
		return nullptr;
	}
	
	if (UClass* LoadedClass = FactoryClass.LoadSynchronous())
	{
		auto Item = MakeShared<FPlaceableItem>(*LoadedClass, AssetData, NAME_None, NAME_None,  TOptional<FLinearColor>(), TOptional<int32>(), TOptional<FText>());
		ConfigurePlaceableItem(this, Item);
		return Item;
	}
	return nullptr;
}

// ======================================================================
// ======================================================================

FConfigPlaceableItem_ActorClass::FConfigPlaceableItem_ActorClass(TSoftClassPtr<AActor> InClass)
	: ActorClass(InClass)
{
	
}

bool FConfigPlaceableItem_ActorClass::IdenticalTo(const FConfigPlaceableItem& Other) const
{
	const FConfigPlaceableItem_ActorClass& LocalOther = static_cast<const FConfigPlaceableItem_ActorClass&>(Other);
	return ActorClass == LocalOther.ActorClass;
}

bool FConfigPlaceableItem_ActorClass::IsValidData() const
{
	return !ActorClass.IsNull();
}

TSharedPtr<FPlaceableItem> FConfigPlaceableItem_ActorClass::MakeItem() const
{
	FAssetData AssetData;
	if (!TryRequestAssetData(ActorClass, AssetData))
	{
		return nullptr;
	}

	TScriptInterface<IAssetFactoryInterface> AssetFactory = nullptr;
	if (TryRequestAssetFactoryForAsset(AssetData, AssetFactory))
	{
		auto Item = MakeShared<FPlaceableItem>(AssetFactory, AssetData, NAME_None, NAME_None, TOptional<FLinearColor>(), TOptional<int32>(), TOptional<FText>());
		ConfigurePlaceableItem(this, Item);
		return Item;
	}

	return nullptr;
}

// ======================================================================
// ======================================================================

FConfigPlaceableItem_AssetObject::FConfigPlaceableItem_AssetObject(TSoftObjectPtr<UObject> InObject)
	: Object(InObject)
{
}

bool FConfigPlaceableItem_AssetObject::IdenticalTo(const FConfigPlaceableItem& Other) const
{
	const FConfigPlaceableItem_AssetObject& LocalOther = static_cast<const FConfigPlaceableItem_AssetObject&>(Other);
	return Object == LocalOther.Object;
}

bool FConfigPlaceableItem_AssetObject::IsValidData() const
{
	return !Object.IsNull();
}

TSharedPtr<FPlaceableItem> FConfigPlaceableItem_AssetObject::MakeItem() const
{
	FAssetData AssetData;
	if (!TryRequestAssetData(Object, AssetData))
	{
		return nullptr;
	}

	TScriptInterface<IAssetFactoryInterface> AssetFactory = nullptr;
	if (TryRequestAssetFactoryForAsset(AssetData, AssetFactory))
	{
		auto Item = MakeShared<FPlaceableItem>(AssetFactory, AssetData, NAME_None, NAME_None, TOptional<FLinearColor>(), TOptional<int32>(), TOptional<FText>());
		ConfigurePlaceableItem(this, Item);
		return Item;
	}

	return nullptr;
}

// ======================================================================
// ======================================================================

FConfigPlaceableItem_AssetData::FConfigPlaceableItem_AssetData(const FAssetData& InAssetData) : AssetData(InAssetData)
{
}

bool FConfigPlaceableItem_AssetData::IdenticalTo(const FConfigPlaceableItem& Other) const
{
	const FConfigPlaceableItem_AssetData& LocalOther = static_cast<const FConfigPlaceableItem_AssetData&>(Other);
	return AssetData == LocalOther.AssetData;
}

bool FConfigPlaceableItem_AssetData::IsValidData() const
{
	return AssetData.IsValid();
}

TSharedPtr<FPlaceableItem> FConfigPlaceableItem_AssetData::MakeItem() const
{
	TScriptInterface<IAssetFactoryInterface> AssetFactory = nullptr;
	if (TryRequestAssetFactoryForAsset(AssetData, AssetFactory))
	{
		auto Item = MakeShared<FPlaceableItem>(AssetFactory, AssetData, NAME_None, NAME_None, TOptional<FLinearColor>(), TOptional<int32>(), TOptional<FText>());
		ConfigurePlaceableItem(this, Item);
		return Item;
	}
	return nullptr;
}

// ======================================================================
// ======================================================================

bool FStandardPlacementCategoryInfo::CanEditChange(const FProperty* InProperty) const
{
	static const FName PROP_SortOrder = GET_MEMBER_NAME_CHECKED(FStandardPlacementCategoryInfo, Order);
	
	const FName PropertyName = InProperty ? InProperty->GetFName() : NAME_None;
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 5, 0)
	if (UniqueId == FBuiltInPlacementCategories::Favorites() && PropertyName == PROP_SortOrder)
		return false;
#endif
	if (UniqueId == FBuiltInPlacementCategories::RecentlyPlaced() && PropertyName == PROP_SortOrder)
		return false;

	return true;
}

bool FStandardPlacementCategoryInfo::CanEditChange(const FEditPropertyChain& PropertyChain) const
{
	return CanEditChange(PropertyChain.GetActiveNode() ? PropertyChain.GetActiveNode()->GetValue() : nullptr);
}

// ======================================================================
// ======================================================================

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

TArray<TSoftObjectPtr<UScriptStruct>> UEnhancedPaletteSettings::GetUsableStaticItems()
{
	TArray<TSoftObjectPtr<UScriptStruct>> Result;
	Result.Add(FConfigPlaceableItem_FactoryClass::StaticStruct());
	Result.Add(FConfigPlaceableItem_FactoryObject::StaticStruct());
	Result.Add(FConfigPlaceableItem_ActorClass::StaticStruct());
	Result.Add(FConfigPlaceableItem_AssetObject::StaticStruct());
	return Result;
}

TArray<TSoftObjectPtr<UScriptStruct>> UEnhancedPaletteSettings::GetUnUsableStaticItems()
{
	TArray<TSoftObjectPtr<UScriptStruct>> Result;
	Result.Add(FConfigPlaceableItem_Native::StaticStruct());
	Result.Add(FConfigPlaceableItem_FactoryAssetData::StaticStruct());
	Result.Add(FConfigPlaceableItem_AssetData::StaticStruct());
	return Result;
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

