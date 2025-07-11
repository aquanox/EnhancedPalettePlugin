// Copyright 2025, Aquanox.

#include "EnhancedPaletteCategory.h"

#include "ActorFactories/ActorFactory.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/Blueprint.h"
#include "EnhancedPaletteLibrary.h"
#include "Textures/SlateIcon.h"
#include "EnhancedPaletteGlobals.h"
#include "EnhancedPaletteSubsystem.h"

void UEnhancedPaletteCategory::PostInitProperties()
{
	Super::PostInitProperties();

	FString StringName = GetClass()->GetName();
	StringName.RemoveFromEnd(TEXT("_C"));
	UniqueId = *StringName;
}

void UEnhancedPaletteCategory::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (UEnhancedPaletteSubsystem* Subsystem = UEnhancedPaletteSubsystem::Get())
	{
		Subsystem->MarkCategoryDirty(GetCategoryUniqueId(), EManagedCategoryDirtyFlags::All);
	}
}

FName UEnhancedPaletteCategory::GetCategoryUniqueId() const
{
	ensure(!UniqueId.IsNone());
	return UniqueId;
}

FText UEnhancedPaletteCategory::GetDisplayName() const
{
	return DisplayName.IsEmpty() ? GetClass()->GetDisplayNameText() : DisplayName;
}

FText UEnhancedPaletteCategory::GetShortDisplayName() const
{
	return ShortDisplayName.IsEmpty() ? GetDisplayName() : ShortDisplayName;
}

FSlateIcon UEnhancedPaletteCategory::GetDisplayIcon() const
{
	return DisplayIcon.ToSlateIcon();
}

FString UEnhancedPaletteCategory::GetTagMetaData() const
{
	if (TagMetaData.IsEmpty())
	{
		return FString(TEXT("PM")) + GetCategoryUniqueId().ToString();
	}
	return TagMetaData;
}

void UEnhancedPaletteCategory::Initialize(UEnhancedPaletteSubsystem* InSubystem)
{
	{
		FEditorScriptExecutionGuard Guard;
		NativeInitialize();
		K2_Initialize();
	}
}

void UEnhancedPaletteCategory::NativeInitialize()
{
}

void UEnhancedPaletteCategory::Tick(float DeltaTime)
{
	if (bTickable)
	{
		Accumulator += DeltaTime;
		if (Accumulator >= TickInterval)
		{
			Accumulator = 0;

			FEditorScriptExecutionGuard Guard;
			NativeTick();
			K2_Tick();
		}
	}
}

void UEnhancedPaletteCategory::NativeTick()
{
}

void UEnhancedPaletteCategory::GatherItems(TArray<TConfigPlaceableItem>& OutResult)
{
	TGuardValue<bool> IsGathering(bGathering, true);

	// reset all previousy registered elements and state
	AutoOrder.Reset();
	LocalDescriptors.Reset();

	{
		FEditorScriptExecutionGuard Guard;

		// native gather
		NativeGatherItems();
		// bp gather, tbd 
		K2_GatherItems();
	}

	OutResult = MoveTemp(LocalDescriptors);
	LocalDescriptors.Reset();
}

void UEnhancedPaletteCategory::NativeGatherItems()
{
}

void UEnhancedPaletteCategory::AddInternal(TConfigPlaceableItem&& Desc)
{
	if (!bGathering)
	{
		FFrame::KismetExecutionMessage(TEXT("Add() can be used only during Gather Items"), ELogVerbosity::Warning);
		return;
	}

	if (!Desc.IsValid() || !Desc.Get<FConfigPlaceableItem>().IsValidData())
	{
		UE_LOG(LogEnhancedPalette, Warning, TEXT("Failed to add descriptor: data is invalid"));
		return;
	}

	if (AutoOrder.IsSet())
	{
		Desc.GetMutable<FConfigPlaceableItem>().SortOrder = ++AutoOrder.GetValue();
	}

	for (const TConfigPlaceableItem& Existing : LocalDescriptors)
	{
		if (Existing.GetScriptStruct() == Desc.GetScriptStruct()
			&& Existing.Get<FConfigPlaceableItem>().IdenticalTo(Desc.Get<FConfigPlaceableItem>()))
		{
			UE_LOG(LogEnhancedPalette, Warning, TEXT("Failed to add descriptor: duplicate"));
			return;
		}
	}

	LocalDescriptors.Emplace(MoveTemp(Desc));
}

void UEnhancedPaletteCategory::AddItem(TInstancedStruct<FConfigPlaceableItem> ItemStruct)
{
	AddInternal(MoveTemp(ItemStruct));
}

void UEnhancedPaletteCategory::AddFactoryClass(TSoftClassPtr<UActorFactory> FactoryClass, FName NativeName, FText ItemName, int32 ItemSortOrder)
{
	FConfigPlaceableItem_FactoryClass Cfg;
	Cfg.FactoryClass = FactoryClass;
	Cfg.NativeName = NativeName;
	Cfg.DisplayName = ItemName;
	Cfg.SortOrder = ItemSortOrder;
	AddInternal(TConfigPlaceableItem::Make<FConfigPlaceableItem_FactoryClass>(Cfg));
}

void UEnhancedPaletteCategory::AddFactoryWithAsset(TSoftClassPtr<UActorFactory> Factory, const FAssetData& AssetData, FName NativeName, FText ItemName, int32 ItemSortOrder)
{
	FConfigPlaceableItem_FactoryAssetData Cfg;
	Cfg.FactoryClass = Factory;
	Cfg.AssetData = AssetData;
	Cfg.NativeName = NativeName;
	Cfg.DisplayName = ItemName;
	Cfg.SortOrder = ItemSortOrder;
	AddInternal(TConfigPlaceableItem::Make<FConfigPlaceableItem_FactoryAssetData>(Cfg));
}

void UEnhancedPaletteCategory::AddFactoryWithObject(TSoftClassPtr<UActorFactory> FactoryClass, TSoftObjectPtr<UObject> AssetObject, FName NativeName, FText ItemName, int32 ItemSortOrder)
{
	FConfigPlaceableItem_FactoryObject Cfg;
	Cfg.FactoryClass = FactoryClass;
	Cfg.Object = AssetObject;
	Cfg.NativeName = NativeName;
	Cfg.DisplayName = ItemName;
	Cfg.SortOrder = ItemSortOrder;
	AddInternal(TConfigPlaceableItem::Make<FConfigPlaceableItem_FactoryObject>(Cfg));
}

void UEnhancedPaletteCategory::AddActorClass(TSoftClassPtr<AActor> ActorClass, FName NativeName, FText ItemName, int32 ItemSortOrder)
{
	FConfigPlaceableItem_ActorClass Cfg;
	Cfg.ActorClass = ActorClass;
	Cfg.NativeName = NativeName;
	Cfg.DisplayName = ItemName;
	Cfg.SortOrder = ItemSortOrder;
	AddInternal(TConfigPlaceableItem::Make<FConfigPlaceableItem_ActorClass>(Cfg));
}

void UEnhancedPaletteCategory::AddAssetObject(TSoftObjectPtr<UObject> AssetObject, FName NativeName, FText ItemName, int32 ItemSortOrder)
{
	FConfigPlaceableItem_AssetObject Cfg;
	Cfg.Object = AssetObject;
	Cfg.NativeName = NativeName;
	Cfg.DisplayName = ItemName;
	Cfg.SortOrder = ItemSortOrder;
	AddInternal(TConfigPlaceableItem::Make<FConfigPlaceableItem_AssetObject>(Cfg));
}

void UEnhancedPaletteCategory::AddAssetData(const FAssetData& AssetData, FName NativeName, FText ItemName, int32 ItemSortOrder)
{
	FConfigPlaceableItem_AssetData Cfg;
	Cfg.AssetData = AssetData;
	Cfg.NativeName = NativeName;
	Cfg.DisplayName = ItemName;
	Cfg.SortOrder = ItemSortOrder;
	AddInternal(TConfigPlaceableItem::Make<FConfigPlaceableItem_AssetData>(Cfg));
}

void UEnhancedPaletteCategory::SetAutoOrder(int32 StartOrder)
{
	if (!bGathering)
	{
		FFrame::KismetExecutionMessage(TEXT("SetAutoOrder() can be used only during Gather Items"), ELogVerbosity::Warning);
		return;
	}

	AutoOrder = StartOrder;
}

void UEnhancedPaletteCategory::NotifyContentChanged()
{
	if (auto* Subsystem = UEnhancedPaletteSubsystem::Get())
	{
		Subsystem->MarkCategoryDirty(GetCategoryUniqueId(), EManagedCategoryDirtyFlags::Content);
	}
}

void UEnhancedPaletteCategory::SortItems()
{
	if (!bGathering)
	{
		FFrame::KismetExecutionMessage(TEXT("SortItems() can be used only during Gather Items"), ELogVerbosity::Warning);
		return;
	}

	Algo::StableSort(LocalDescriptors, FConfigPlaceableItemSorter());
}

void UEnhancedPaletteCategory::PrintDebugInfo()
{
	UE_LOG(LogEnhancedPalette, Log,
	       TEXT("DEBUG %08x this=%p %s class=%p %s t=%s"),
	       GetUniqueID(), this, *GetName(),
	       GetClass(), *GetClass()->GetName(),
	       *GetTagMetaData());
}
