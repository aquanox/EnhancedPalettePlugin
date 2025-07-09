// Copyright 2025, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "ActorPlacementInfo.h"
#include "IPlacementModeModule.h"
#include "Misc/NamePermissionList.h"

struct FConfigActorPlacementInfo;

class SWidget;

struct FPermissionListAccess : public FNamePermissionList
{
	friend class UEnhancedPaletteSubsystem;
	inline void NotifyChanged() { OnFilterChanged().Broadcast(); }
};

/**
 * Helper that wraps access to IPlacementModeModule
 */
struct FPlacementModeModuleAccess
{
	FPlacementModeModuleAccess(IPlacementModeModule& Original);
private:
	FPlacementModeModuleAccess(const FPlacementModeModuleAccess& ) = delete;
	FPlacementModeModuleAccess& operator=(const FPlacementModeModuleAccess&) = delete;
public:
	IPlacementModeModule& Get() const;
	IPlacementModeModule* operator->() const { return &Get(); }
	
	/** 
	 * Get names of all standard engine categories
	 */
	static const TArray<FName>& GetBuiltInCategoryNames();
	static bool IsBuiltinCategory(const FName& InName);

	// Get all registered category names, not just filtered ones
	TArray<FName> GetRegisteredCategoryNames();

	void NotifyCategoriesChanged();
	void NotifyCategoryRefreshed(const FName& InName);
	void NotifyRecentChanged();
	void NotifyAssetsChanged();
	
	void SetRecentList(const TArray<FConfigActorPlacementInfo>& NewList);
	
	// @hack for 5.5+
	TSharedPtr<SWidget> TryDiscoverToolWidget();
	void TryForceToolbarRefresh();
	void TryForceContentRefresh();
	
private:
	IPlacementModeModule* Impl = nullptr;
	struct FriendlyPM& GetImpl() const;

	TWeakPtr<SWidget> PlacementBrowserToolbarWidget;
};
