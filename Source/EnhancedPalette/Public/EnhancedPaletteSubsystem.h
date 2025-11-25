// Copyright 2025, Aquanox.

#pragma once

#include "EditorSubsystem.h"
#include "AssetRegistry/AssetData.h"
#include "EnhancedPaletteGlobals.h"
#include "EnhancedPaletteSettings.h"
#include "UObject/ObjectKey.h"
#include "IPlacementModeModule.h"
#include "TickableEditorObject.h"
#include "Misc/EnumClassFlags.h"
#include "EnhancedPaletteSubsystem.generated.h"

struct FManagedCategory;
struct FManagedCategoryChangeTracker;
struct FPlacementModeModuleAccess;

enum class EManagedCategoryFlags
{
	Type_Config = 0x01,
	Type_Asset = 0x02,
	Type_External = 0x04,
	Type_Any = 0x0F,

	// State_Registered = 0x10,
	// State_DirtyInfo = 0x20,
	// State_DirtyContent = 0x40,
	// State_DirtyInstance = 0x80,

	DynamicTrait_Blueprint = 0x100,
	DynamicTrait_Asset = 0x200,
	DynamicTrait_World = 0x400,
	DynamicTrait_Interval = 0x800,
};

ENUM_CLASS_FLAGS(EManagedCategoryFlags);

enum class EManagedCategoryDirtyFlags
{
	None = 0,
	Content = 1,
	Info = 2,
	//Instance = 4,
	All = 15,
};

ENUM_CLASS_FLAGS(EManagedCategoryDirtyFlags);


/**
 * Core of palette customizer plugin.
 *
 */
UCLASS()
class ENHANCEDPALETTE_API UEnhancedPaletteSubsystem : public UEditorSubsystem, public FTickableEditorObject
{
	GENERATED_BODY()

public:
	// Quick subsystem accessor
	static UEnhancedPaletteSubsystem* Get();

	// {{{ Tickable
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickable() const override;
	virtual void Tick(float DeltaTime) override;
	// }}}

	// {{{ Subsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// }}}

	// {{{ UObject
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// }}}

	// {{{ editor tracking
	void TrySetupPlacementModule(FName, EModuleChangeReason);
	void OnInitialAssetsScanComplete();
	// }}}

	// {{{ data
	TSharedPtr<FManagedCategory> FindManagedCategory(const FName& InId) const;
	void MarkCategoryDirty(FName UniqueId, EManagedCategoryDirtyFlags DirtyFlags = EManagedCategoryDirtyFlags::Content);
	void MarkCategoryDirty(EManagedCategoryFlags Trait, EManagedCategoryDirtyFlags DirtyFlags = EManagedCategoryDirtyFlags::Content);
	// }}}

	// {{{ discovery
	void TryDiscoverCategories();
	void TryDiscoverFromConfig(TArray<TSharedPtr<FManagedCategory>>& OutCategories) const;
	void TryDiscoverFromNativeScan(TArray<TSharedPtr<FManagedCategory>>& OutCategories) const;
	void TryDiscoverFromAssetScan(TArray<TSharedPtr<FManagedCategory>>& OutCategories) const;

	void TryPopulateCategoryItems();
	// }}}

	// {{{ externals
	bool CreateExternalCategory(const FStaticPlacementCategoryInfo& CreationInfo);
	bool RemoveExternalCategory(const FName& UniqueId);
	bool AddExternalCategoryItem(const FName& UniqueId, TInstancedStruct<FConfigPlaceableItem> Item);
	// }}}

	// called when visibility settings changed and need to be applied to PM
	void ApplyEngineCategorySettings();
	// called when category settings changed and need to be applied to PM
	void ApplyManagedCategorySettings();
	// called when recent list settings changed and need to be applied to PM
	void ApplyRecentListSettings();
	// force settings save
	void TrySaveSettings();

	void OnPlacementModeCategoryListChanged();
	void OnPlaceableItemFilteringChanged();
	void OnAllPlaceableAssetsChanged();
	void OnRecentlyPlacedChanged(const TArray<FActorPlacementInfo>&);

	void OnSettingsPanelSelected(); // load PM data onto settings panel
	void OnSettingsPanelModified(UObject*, FPropertyChangedEvent&);
	void OnSettingsPanelCommand(FName CommandId);

	void OnCategoryBlueprintModified(class UBlueprint*, FName CategoryId);
	void OnCategoryObjectModified(UObject*, struct FPropertyChangedEvent&, FName Category);

	using FOnPlacementModuleReady = TMulticastDelegate<void(IPlacementModeModule&)>;
	FOnPlacementModuleReady& OnPlacementModuleReady() { return OnPlacementModuleReadyPrivate; }

protected:
	// All managed categories stored here
	TArray<TSharedPtr<FManagedCategory>> ManagedCategories;

	TSharedPtr<FPlacementModeModuleAccess> ModuleAccessPrivate;

	FOnPlacementModuleReady OnPlacementModuleReadyPrivate;

	FPlacementModeModuleAccess& GetModuleRef() const
	{
		check(ModuleAccessPrivate.IsValid());
		return *ModuleAccessPrivate;
	}

	TSharedPtr<FManagedCategoryChangeTracker> ExternalChangeTracker;

	TWeakPtr<class ISettingsSection> SettingsSectionPtr;

	TWeakPtr<class SNotificationItem> NotificationItemPtr;

	// flag that indicates that subsystem is waiting for asset load to complete
	bool bPendingAssetLoad = false;

	// flag that indicates subsystem successful initialization
	bool bSubsystemReady = false;

	bool bRequireDiscover = false;
	bool bRequirePopulate = false;
	bool bRequireUpdateManagedCategories = false;
	bool bRequireUpdateEngineCategories = false;
	bool bRequireApplyRecentList = false;
	bool bRequireSettingsSave = false;
	bool bRequireToolbarRefresh = false;
	bool bRequireToolbarContentRefresh = false;
public:
	inline void RequestDiscover() { bRequireDiscover = true; }
	inline void RequestPopulate() { bRequirePopulate = true; }
	inline void RequestUpdateCategoryData() { bRequireUpdateManagedCategories = true; bRequireUpdateEngineCategories = true; }
	inline void RequestSettingsSave() { bRequireSettingsSave = true; }
	inline void RequestRecentListSave() { bRequireApplyRecentList = true; }
	inline void RequestToolbarRefresh() { bRequireToolbarRefresh = true; }
	inline void RequestToolbarContentRefresh() { bRequireToolbarContentRefresh = true; }
};
