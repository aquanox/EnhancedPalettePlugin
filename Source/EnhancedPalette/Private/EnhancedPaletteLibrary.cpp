// Copyright 2025, Aquanox.


#include "EnhancedPaletteLibrary.h"
#include "EnhancedPaletteSubsystem.h"
#include "EnhancedPaletteCategory.h"

void UEnhancedPaletteLibrary::RequestDiscoverCategories()
{
	if (auto* Subsystem = UEnhancedPaletteSubsystem::Get())
	{
		return Subsystem->RequestDiscover();
	}
}

void UEnhancedPaletteLibrary::RequestPopulateCategories()
{
	if (auto* Subsystem = UEnhancedPaletteSubsystem::Get())
	{
		return Subsystem->RequestPopulate();
	}
}

bool UEnhancedPaletteLibrary::RegisterExternalCategory(FName UniqueId, FText DisplayName, FText ShortDisplayName, FString DisplayIconCode, int32 SortOrder, bool bSortable)
{
	if (UniqueId.IsNone())
	{
		UE_LOG(LogEnhancedPalette, Warning, TEXT("Failed to create external category: bad identifier"))
		return false;
	}

	if (auto* Subsystem = UEnhancedPaletteSubsystem::Get())
	{
		if (DisplayName.IsEmptyOrWhitespace())
			DisplayName = FText::FromName(UniqueId);
		if (ShortDisplayName.IsEmptyOrWhitespace())
			ShortDisplayName = DisplayName;
		if (DisplayIconCode.IsEmpty() || !DisplayIconCode.Contains(TEXT(":")))
			DisplayIconCode = FAppStyle::GetAppStyleSetName().ToString() + TEXT("PlacementBrowser.Icons.Basic");

		FStaticPlacementCategoryInfo Info;
		Info.UniqueId = UniqueId;
		Info.DisplayName = DisplayName;
		Info.ShortDisplayName = ShortDisplayName;
		Info.DisplayIcon = FSimpleIconSelector { DisplayIconCode };
		Info.SortOrder = SortOrder;
		Info.bSortable = bSortable;

		Subsystem->CreateExternalCategory(Info);
		return true;
	}
	return false;
}

bool UEnhancedPaletteLibrary::UnregisterExternalCategory(FName UniqueId)
{
	if (auto* Subsystem = UEnhancedPaletteSubsystem::Get())
	{
		return Subsystem->RemoveExternalCategory(UniqueId);
	}
	return true;
}

bool UEnhancedPaletteLibrary::AddCategoryPlaceableItem(FName UniqueId, TInstancedStruct<FConfigPlaceableItem> Descriptor)
{
	if (auto* Subsystem = UEnhancedPaletteSubsystem::Get())
	{
		// TBD: maybe allow adding items to others?
		Subsystem->AddExternalCategoryItem(UniqueId, Descriptor);
	}
	return true;
}

void UEnhancedPaletteLibrary::NotifyCategoryChanged(UEnhancedPaletteCategory* Category, bool bContent, bool bInfo)
{
	auto* Subsystem = UEnhancedPaletteSubsystem::Get();
	if (Subsystem && IsValid(Category))
	{
		EManagedCategoryDirtyFlags Flags = EManagedCategoryDirtyFlags::None;
		if (bContent) Flags |= EManagedCategoryDirtyFlags::Content;
		if (bInfo) Flags |= EManagedCategoryDirtyFlags::Info;
		//if (bInstance)	Flags |= EManagedCategoryDirtyFlags::Instance;
		if (Flags != EManagedCategoryDirtyFlags::None)
		{
			Subsystem->MarkCategoryDirty(Category->GetCategoryUniqueId(), Flags);
		}
	}
}

UActorFactory* UEnhancedPaletteLibrary::FindActorFactory(TSubclassOf<UActorFactory> Class)
{
	return GEditor->FindActorFactoryByClass(Class);
}

UActorFactory* UEnhancedPaletteLibrary::FindActorFactoryForActor(TSubclassOf<AActor> Class)
{
	return GEditor->FindActorFactoryForActorClass(Class);
}

UActorFactory* UEnhancedPaletteLibrary::FindActorFactoryByClassForActor(TSubclassOf<UActorFactory> FactoryClass, TSubclassOf<AActor> ActorClass)
{
	return GEditor->FindActorFactoryByClassForActorClass(FactoryClass, ActorClass);
}
