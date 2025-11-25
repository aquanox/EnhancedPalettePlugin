// Copyright 2025, Aquanox.

#pragma once

#include "PlacementModeModuleAccess.h"
#include "IPlacementModeModule.h"
#include "ActorPlacementInfo.h"
#include "PrivateAccessHelper.h"
#include "EnhancedPaletteGlobals.h"
#include "EnhancedPaletteSettings.h"
#include "Misc/NamePermissionList.h"
#include "LevelEditor.h"
#include "TutorialMetaData.h"
#include "Widgets/Docking/SDockTab.h"
#include "Misc/ConfigCacheIni.h"

#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 5, 0)
#include "Layout/CategoryDrivenContentBuilderBase.h"
#include "Layout/CategoryDrivenContentBuilder.h"
#endif

#pragma push_macro("IMPLEMENT_MODULE")
#undef IMPLEMENT_MODULE
#define IMPLEMENT_MODULE(...)
// @from public
#include "PlacementModeModule.h"
// @from private
#include "SPlacementModeTools.h"
#pragma pop_macro("IMPLEMENT_MODULE")

#define DECLARE_DERIVED_H_EVENT( OwningType, BaseTypeEvent, EventName ) \
	class EventName : public BaseTypeEvent { friend class FPlacementModeModuleAccess; };

struct FriendlyPM : public FPlacementModeModule
{
	// and now it can access guts
	friend struct FPlacementModeModuleAccess;
	// DECLARE_DERIVED_H_EVENT(FriendlyPM, FPlacementModeModule::FOnPlacementModeCategoryListChanged, FOnPlacementModeCategoryListChanged);
	// DECLARE_DERIVED_H_EVENT(FriendlyPM, FPlacementModeModule::FOnPlacementModeCategoryRefreshed, FOnPlacementModeCategoryRefreshed);
	// DECLARE_DERIVED_H_EVENT(FriendlyPM, FPlacementModeModule::FOnRecentlyPlacedChanged, FOnRecentlyPlacedChanged);
	// DECLARE_DERIVED_H_EVENT(FriendlyPM, FPlacementModeModule::FOnAllPlaceableAssetsChanged, FOnAllPlaceableAssetsChanged);
	// DECLARE_DERIVED_H_EVENT(FriendlyPM, FPlacementModeModule::FOnPlaceableItemFilteringChanged, FOnPlaceableItemFilteringChanged);
};

using PlacementCategoryMap = TMap<FName, FPlacementCategory>;
UE_DEFINE_PRIVATE_MEMBER_PTR(PlacementCategoryMap, GCategories, FPlacementModeModule, Categories);

using FOnPlacementModeCategoryListChanged = FriendlyPM::FOnPlacementModeCategoryListChanged;
UE_DEFINE_PRIVATE_MEMBER_PTR(FOnPlacementModeCategoryListChanged, GFOnPlacementModeCategoryListChanged, FriendlyPM, PlacementModeCategoryListChanged);

#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 5, 0)
using FCategoryContentBuilderPtr = TSharedPtr<FCategoryDrivenContentBuilder>;
UE_DEFINE_PRIVATE_MEMBER_PTR(FCategoryContentBuilderPtr, GToolsCategoryContentBuilder, SPlacementModeTools, CategoryContentBuilder);
#else
UE_DEFINE_PRIVATE_MEMBER_PTR(bool, GUpdateShownItems, SPlacementModeTools, bUpdateShownItems);
#endif

using FRecentListArray = TArray<FActorPlacementInfo>;
UE_DEFINE_PRIVATE_MEMBER_PTR(FRecentListArray, GRecentlyPlaced, FriendlyPM, RecentlyPlaced);

FPlacementModeModuleAccess::FPlacementModeModuleAccess(IPlacementModeModule& Original)
{
	Impl = &Original;
}

IPlacementModeModule& FPlacementModeModuleAccess::Get() const
{
	ensure(Impl != nullptr);
	return *Impl;
}

FriendlyPM& FPlacementModeModuleAccess::GetImpl() const
{
	ensure(Impl != nullptr);
	return *reinterpret_cast<FriendlyPM*>(Impl);
}

const TArray<FName>& FPlacementModeModuleAccess::GetBuiltInCategoryNames()
{
	static const TArray<FName> Values = {
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 5, 0)
		FBuiltInPlacementCategories::Favorites(),
#endif
		FBuiltInPlacementCategories::RecentlyPlaced(),
		FBuiltInPlacementCategories::AllClasses(),
		FBuiltInPlacementCategories::Basic(),
		FBuiltInPlacementCategories::Lights(),
		FBuiltInPlacementCategories::Shapes(),
		FBuiltInPlacementCategories::Visual(),
		FBuiltInPlacementCategories::Volumes()
	};
	return Values;
}

bool FPlacementModeModuleAccess::IsBuiltinCategory(const FName& InName)
{
	return GetBuiltInCategoryNames().Contains(InName);
}

TArray<FName> FPlacementModeModuleAccess::GetRegisteredCategoryNames()
{
	TArray<FName> Keys;
	(GetImpl().*GCategories).GetKeys(Keys);
	return Keys;
}

void FPlacementModeModuleAccess::NotifyCategoriesChanged()
{
	GetImpl().OnPlacementModeCategoryListChanged().Broadcast();
	TryForceToolbarRefresh();
}

void FPlacementModeModuleAccess::NotifyCategoryRefreshed(const FName& InName)
{
	GetImpl().OnPlacementModeCategoryRefreshed().Broadcast(InName);
}

void FPlacementModeModuleAccess::NotifyRecentChanged()
{
	const TArray<FActorPlacementInfo>& Recent = GetImpl().GetRecentlyPlaced();
	GetImpl().OnRecentlyPlacedChanged().Broadcast(Recent);
}

void FPlacementModeModuleAccess::NotifyAssetsChanged()
{
	GetImpl().OnAllPlaceableAssetsChanged().Broadcast();
}

void FPlacementModeModuleAccess::SetRecentList(const TArray<FConfigActorPlacementInfo>& NewList)
{
	// There are only two options - new list will have some element removed or have no elements at all
	TArray<FActorPlacementInfo>& RecentlyPlaced = GetImpl().*GRecentlyPlaced;

	TArray<FString> RecentlyPlacedAsStrings;

	if (NewList.IsEmpty()) // user clicked to clear recent list
	{
		RecentlyPlaced.Reset();
	}
	else // some entries were removed
	{
		for (auto It = RecentlyPlaced.CreateIterator(); It; ++It)
		{
			const FActorPlacementInfo& Info = *It;

			bool bRemain = NewList.FindByPredicate([&Info](const FConfigActorPlacementInfo& Elem)
			{
				return Elem.Factory == Info.Factory && Elem.ObjectPath == Info.ObjectPath;
			}) != nullptr;

			if (!bRemain)
			{
				It.RemoveCurrent();
				continue;
			}

			RecentlyPlacedAsStrings.Add(Info.ToString());
		}
	}

	GConfig->SetArray(TEXT("PlacementMode"), TEXT("RecentlyPlaced"), RecentlyPlacedAsStrings, GEditorPerProjectIni);

	GetImpl().OnRecentlyPlacedChanged().Broadcast(RecentlyPlaced);
}

TSharedPtr<SWidget> FPlacementModeModuleAccess::TryDiscoverToolWidget()
{
	if (!PlacementBrowserToolbarWidget.IsValid())
	{
		FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
		if (TSharedPtr<SDockTab> DockTab = LevelEditor.GetLevelEditorTabManager()->FindExistingLiveTab(FTabId(LevelEditorTabIds::PlacementBrowser)))
		{
			checkf(DockTab->GetMetaData<FTutorialMetaData>()->Tag == "PlacementBrowser", TEXT("Must be of a PB type"));
			TSharedRef<SWidget> Widget = DockTab->GetContent();
			checkf(Widget->GetType() == "SPlacementModeTools", TEXT("Must be of a PB type"));
			PlacementBrowserToolbarWidget = Widget;
			return Widget;
		}
	}
	return PlacementBrowserToolbarWidget.Pin();
}

void FPlacementModeModuleAccess::TryForceToolbarRefresh()
{
	// epic does not refresh toolbar properly after initial populate, so new/changed things do not reflect
	// to fix that need to locate the PlacementMode widget and
	if (auto Widget = TryDiscoverToolWidget())
	{
		TSharedRef<SPlacementModeTools> ToolsWidgetPrivate = StaticCastSharedRef<SPlacementModeTools>(Widget.ToSharedRef());
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 5, 0)
		FCategoryContentBuilderPtr ToolsBuilderPrivate = (*ToolsWidgetPrivate).*GToolsCategoryContentBuilder;
		// force toolbar refresh
		ToolsBuilderPrivate->RefreshCategoryToolbarWidget(true);
#endif
	}
}

void FPlacementModeModuleAccess::TryForceContentRefresh()
{
	// epic does not refresh content properly when category is being selected, so need to
	if (auto Widget = TryDiscoverToolWidget())
	{
		TSharedRef<SPlacementModeTools> ToolsWidgetPrivate = StaticCastSharedRef<SPlacementModeTools>(Widget.ToSharedRef());
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 5, 0)
		FCategoryContentBuilderPtr ToolsBuilderPrivate = (*ToolsWidgetPrivate).*GToolsCategoryContentBuilder;
		// force content refresh
		ToolsBuilderPrivate->UpdateWidget();
#else
		(*ToolsWidgetPrivate).*GUpdateShownItems = true;
#endif
	}
}
