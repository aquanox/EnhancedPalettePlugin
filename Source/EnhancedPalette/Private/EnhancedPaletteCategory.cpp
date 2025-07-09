// Copyright 2025, Aquanox.

#include "EnhancedPaletteCategory.h"

#include "ActorFactories/ActorFactory.h"
#include "ActorFactories/ActorFactoryClass.h"
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

void UEnhancedPaletteCategory::GatherItems(TArray<TConfigPlaceableItem>& OutResult)
{
	TGuardValue<bool> IsGathering(bGathering, true);
	
	// reset all previousy registered elements and state
	AutoOrder.Reset();
	Descriptors.Reset();
	
	{
		FEditorScriptExecutionGuard Guard;

		// native gather
		NativeGatherItems(OutResult);
		// bp gather, tbd 
		K2_GatherItems();
	}

	if (bSortable)
	{
		Algo::StableSort(Descriptors, FConfigPlaceableItemSorter());
	}

	OutResult = MoveTemp(Descriptors);
	Descriptors.Reset();
}

void UEnhancedPaletteCategory::AddDescriptorInternal(TConfigPlaceableItem&& Desc)
{
	ensureMsgf(bGathering, TEXT("AddDescriptor was called outside of Gather Items"));
	
	if (!Desc.IsValid() || !Desc.GetPtr<FConfigPlaceableItem>()->IsValidData())
	{
		UE_LOG(LogEnhancedPalette, Warning, TEXT("Failed to add descriptor"));
		return;
	}

	if (AutoOrder.IsSet())
	{
		Desc.GetMutable<FConfigPlaceableItem>().SortOrder = ++AutoOrder.GetValue();
	}

	for (const TConfigPlaceableItem& Existing : Descriptors)
	{
		auto& Left = Existing.Get<FConfigPlaceableItem>();
		auto& Right = Desc.Get<FConfigPlaceableItem>();
		if (Existing.GetScriptStruct() == Desc.GetScriptStruct() && Left.Identical(Right))
		{
			UE_LOG(LogEnhancedPalette, Warning, TEXT("Failed to add descriptor due to duplicate"));
			return;
		}
	}

	Descriptors.Emplace(MoveTemp(Desc));
}

void UEnhancedPaletteCategory::AddDescriptorStruct(TInstancedStruct<FConfigPlaceableItem> ItemStruct)
{
	AddDescriptorInternal(MoveTemp(ItemStruct));
}

void UEnhancedPaletteCategory::AddDescriptor_FactoryClass(TSoftClassPtr<UActorFactory> FactoryClass, FText ItemName, int32 ItemSortOrder)
{
	FConfigPlaceableItem_FactoryClass Cfg;
	Cfg.FactoryClass = FactoryClass;
	Cfg.DisplayName = ItemName;
	Cfg.SortOrder = ItemSortOrder;
	AddDescriptorInternal(TConfigPlaceableItem::Make<FConfigPlaceableItem_FactoryClass>(Cfg));
}

void UEnhancedPaletteCategory::AddDescriptor_FactoryAsset(TSoftClassPtr<UActorFactory> Factory, const FAssetData& AssetData, FText ItemName, int32 ItemSortOrder)
{
	FConfigPlaceableItem_FactoryAsset Cfg;
	Cfg.FactoryClass = Factory;
	Cfg.ObjectData = AssetData;
	Cfg.DisplayName = ItemName;
	Cfg.SortOrder = ItemSortOrder;
	AddDescriptorInternal(TConfigPlaceableItem::Make<FConfigPlaceableItem_FactoryAsset>(Cfg));
}

void UEnhancedPaletteCategory::AddDescriptor_FactoryObject(TSoftClassPtr<UActorFactory> FactoryClass, TSoftObjectPtr<UObject> AssetObject, FText ItemName, int32 ItemSortOrder)
{
	FConfigPlaceableItem_FactoryObject Cfg;
	Cfg.FactoryClass = FactoryClass;
	Cfg.Object = AssetObject;
	Cfg.DisplayName = ItemName;
	Cfg.SortOrder = ItemSortOrder;
	AddDescriptorInternal(TConfigPlaceableItem::Make<FConfigPlaceableItem_FactoryObject>(Cfg));
}

void UEnhancedPaletteCategory::AddDescriptor_ActorClass(TSoftClassPtr<AActor> ActorClass, FText ItemName, int32 ItemSortOrder)
{
	FConfigPlaceableItem_ActorClass Cfg;
	Cfg.ActorClass = ActorClass;
	Cfg.DisplayName = ItemName;
	Cfg.SortOrder = ItemSortOrder;
	AddDescriptorInternal(TConfigPlaceableItem::Make<FConfigPlaceableItem_ActorClass>(Cfg));
}

void UEnhancedPaletteCategory::AddDescriptor_AssetObject(TSoftObjectPtr<UObject> AssetObject, FText ItemName, int32 ItemSortOrder)
{
	FConfigPlaceableItem_AssetObject Cfg;
	Cfg.Object = AssetObject;
	Cfg.DisplayName = ItemName;
	Cfg.SortOrder = ItemSortOrder;
	AddDescriptorInternal(TConfigPlaceableItem::Make<FConfigPlaceableItem_AssetObject>(Cfg));
}

void UEnhancedPaletteCategory::SetAutoOrder(int32 StartOrder)
{
	AutoOrder = StartOrder;
}

void UEnhancedPaletteCategory::NotifyContentChanged()
{
	if (auto* Subsystem = UEnhancedPaletteSubsystem::Get())
	{
		Subsystem->MarkCategoryDirty(GetCategoryUniqueId(), EManagedCategoryDirtyFlags::Content);
	}
}

void UEnhancedPaletteCategory::PrintDebugInfo()
{
	UE_LOG(LogEnhancedPalette, Log,
		TEXT("DEBUG %08x this=%p %s class=%p %s t=%s"),
		GetUniqueID(), this, *GetName(),
		GetClass(), *GetClass()->GetName(),
		*GetTagMetaData());
}
