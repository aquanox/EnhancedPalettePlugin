// Copyright 2025, Aquanox.

#include "EnhancedPaletteSubsystem.h"

#include "ActorFactories/ActorFactoryClass.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EnhancedPaletteCategory.h"
#include "EnhancedPaletteGlobals.h"
#include "EnhancedPaletteLibrary.h"
#include "EnhancedPaletteModule.h"
#include "EnhancedPaletteSettings.h"
#include "EnhancedPaletteSubsystemPrivate.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "LevelEditor.h"
#include "Misc/ConfigCacheIni.h"
#include "PlacementModeModuleAccess.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "Subsystems/PlacementSubsystem.h"
#include "Widgets/SWidget.h"
#include "HAL/IConsoleManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnhancedPaletteSubsystem)

static FAutoConsoleCommand EPP_DiscoverCategories(
	TEXT("EPP.DiscoverCategories"),
	TEXT("Request discover palette categories"),
	FConsoleCommandDelegate::CreateLambda([]() {
		UEnhancedPaletteSubsystem::Get()->OnSettingsPanelCommand(SettingsCommand::DiscoverCategories);
	})
);
static FAutoConsoleCommand EPP_PopulateCategories(
	TEXT("EPP.PopulateCategories"),
	TEXT("Request update category content"),
	FConsoleCommandDelegate::CreateLambda([]() {
		UEnhancedPaletteSubsystem::Get()->OnSettingsPanelCommand(SettingsCommand::PopulateCategories);
	})
);
static FAutoConsoleCommand EPP_UpdateCategores(
	TEXT("EPP.UpdateCategores"),
	TEXT("Request update category details"),
	FConsoleCommandDelegate::CreateLambda([]() {
		UEnhancedPaletteSubsystem::Get()->OnSettingsPanelCommand(SettingsCommand::UpdateCategores);
	})
);
static FAutoConsoleCommand EPP_UpdateToolbar(
	TEXT("EPP.UpdateToolbar"),
	TEXT("Request update toolbar widget"),
	FConsoleCommandDelegate::CreateLambda([]() {
		UEnhancedPaletteSubsystem::Get()->OnSettingsPanelCommand(SettingsCommand::UpdateToolbar);
	})
);
static FAutoConsoleCommand EPP_ClearRecent(
	TEXT("EPP.ClearRecent"),
	TEXT("Request clear recently placed actors list"),
	FConsoleCommandDelegate::CreateLambda([]() {
		UEnhancedPaletteSubsystem::Get()->OnSettingsPanelCommand(SettingsCommand::ClearRecent);
	})
);

UEnhancedPaletteSubsystem* UEnhancedPaletteSubsystem::Get()
{
	return GEditor->GetEditorSubsystem<UEnhancedPaletteSubsystem>();
}

TStatId UEnhancedPaletteSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FEnhancedPaletteSubsystemTicker, STATGROUP_Tickables);
}

ETickableTickType UEnhancedPaletteSubsystem::GetTickableTickType() const
{
	return ETickableTickType::Conditional;
}

bool UEnhancedPaletteSubsystem::IsTickable() const
{
	return !IsTemplate() && IsValid(this) && bSubsystemReady && ModuleAccessPrivate.IsValid();
}

void UEnhancedPaletteSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogEnhancedPalette, Verbose, TEXT("Initializing subsystem"));

	// # ensure asset SS is initialized
	Collection.InitializeDependency<UEditorAssetSubsystem>();
	Collection.InitializeDependency<UPlacementSubsystem>();

	// # Settings setup 

	UEnhancedPaletteSettings* Settings = GetMutableDefault<UEnhancedPaletteSettings>();
	//GConfig->LoadFile(Settings->GetClass()->GetConfigName());
	//Settings->LoadConfig();
	Settings->OnUserAction.BindUObject(this, &ThisClass::OnSettingsPanelCommand);

	ISettingsModule& SettingsModule = FModuleManager::LoadModuleChecked<ISettingsModule>("Settings");
	auto Section = SettingsModule.RegisterSettings(
		Settings->GetContainerName(),
		Settings->GetCategoryName(),
		Settings->GetSectionName(),
		Settings->GetSectionText(),
		Settings->GetSectionDescription(),
		Settings);
	Section->OnSelect().BindUObject(this, &UEnhancedPaletteSubsystem::OnSettingsPanelSelected);
	SettingsSectionPtr = Section;

	Settings->OnSettingChanged().AddUObject(this, &UEnhancedPaletteSubsystem::OnSettingsPanelModified);

	// # Asset tracking setup

	IAssetRegistry& AssetRegistry = IAssetRegistry::GetChecked();

	if (AssetRegistry.IsLoadingAssets())
	{
		// Engine still loading assets - need defer discovery of category assets
		bPendingAssetLoad = true;
		AssetRegistry.OnFilesLoaded().AddUObject(this, &ThisClass::OnInitialAssetsScanComplete);
	}

	// # Placement mode module load awaiting - can not force module to load early as it wrecks PM discovery process 

	// try access PM right away
	TrySetupPlacementModule(NAME_None, EModuleChangeReason::ModuleLoaded);
	if (!ModuleAccessPrivate.IsValid())
	{
		// module is not ready yet - need to wait for it
		FModuleManager::Get().OnModulesChanged().AddUObject(this, &ThisClass::TrySetupPlacementModule);
	}
}

void UEnhancedPaletteSubsystem::TrySetupPlacementModule(FName Name, EModuleChangeReason)
{
	// await until PlacementModule is available naturally or things break if it is forced to load too early 
	if (IPlacementModeModule::IsAvailable() && !ModuleAccessPrivate.IsValid())
	{
		UE_LOG(LogEnhancedPalette, Log, TEXT("Detected PlacementMode module presence. Setting up accessor"));

		ModuleAccessPrivate = MakeShared<FPlacementModeModuleAccess>(IPlacementModeModule::Get());
		// TBD: It is possible other code adds categories in runtime. possibly need to refresh internal list
		(*ModuleAccessPrivate)->OnPlacementModeCategoryListChanged().AddUObject(this, &ThisClass::OnPlacementModeCategoryListChanged);
		// The external placeable item filter can alter content of custom categories. 
		(*ModuleAccessPrivate)->OnPlaceableItemFilteringChanged().AddUObject(this, &ThisClass::OnPlaceableItemFilteringChanged);
		// TBD: PM already tracks most common IAssetRegistry actions.
		// even if multiple refresh triggers occur - the update will happen next tick only
		(*ModuleAccessPrivate)->OnAllPlaceableAssetsChanged().AddUObject(this, &ThisClass::OnAllPlaceableAssetsChanged);
		(*ModuleAccessPrivate)->OnRecentlyPlacedChanged().AddUObject(this, &ThisClass::OnRecentlyPlacedChanged);

		FModuleManager::Get().OnModulesChanged().RemoveAll(this);

		//
		OnPlacementModuleReadyPrivate.Broadcast(IPlacementModeModule::Get());

		// import settings data from PM
		OnSettingsPanelSelected();
		// register delegates
		ExternalChangeTracker = MakeShared<FManagedCategoryChangeTracker>();
		ExternalChangeTracker->RegisterTrackers(this);

		// now we're ready to do anything with PM
		bSubsystemReady = true;
		// and attemp to do first dicover and apply recent list (unrelated to categories)
		RequestDiscover();
		// apply engine category visiblity and order directly
		ApplyEngineCategorySettings();
	}
}

void UEnhancedPaletteSubsystem::OnInitialAssetsScanComplete()
{
	UE_LOG(LogEnhancedPalette, Verbose, TEXT("OnInitialAssetsScanComplete"));
	ensure(bPendingAssetLoad);
	bPendingAssetLoad = false;
}

TSharedPtr<FManagedCategory> UEnhancedPaletteSubsystem::FindManagedCategory(const FName& InId) const
{
	for (const TSharedPtr<FManagedCategory>& Ptr : ManagedCategories)
	{
		if (Ptr->UniqueId == InId)
		{
			return Ptr;
		}
	}
	return nullptr;
}

void UEnhancedPaletteSubsystem::MarkCategoryDirty(FName UniqueId, EManagedCategoryDirtyFlags DirtyFlags)
{
	if (auto Found = FindManagedCategory(UniqueId))
	{
		if (EnumHasAnyFlags(DirtyFlags, EManagedCategoryDirtyFlags::Content))
		{
			Found->bDirtyContent = true;
			RequestPopulate();
		}

		if (EnumHasAnyFlags(DirtyFlags, EManagedCategoryDirtyFlags::Info))
		{
			Found->bDirtyInfo = true;
			RequestUpdateCategoryData();
		}
	}
}

void UEnhancedPaletteSubsystem::MarkCategoryDirty(EManagedCategoryFlags Trait, EManagedCategoryDirtyFlags DirtyFlags)
{
	for (const TSharedPtr<FManagedCategory>& Ptr : ManagedCategories)
	{
		if (Ptr->HasFlag(Trait))
		{
			if (EnumHasAnyFlags(DirtyFlags, EManagedCategoryDirtyFlags::Content))
			{
				Ptr->bDirtyContent = true;
				RequestPopulate();
			}

			if (EnumHasAnyFlags(DirtyFlags, EManagedCategoryDirtyFlags::Info))
			{
				Ptr->bDirtyInfo = true;
				RequestUpdateCategoryData();
			}
		}
	}
}

void UEnhancedPaletteSubsystem::Tick(float DeltaTime)
{
	// do nothing if still waiting for assets to be ready
	if (bPendingAssetLoad || !bSubsystemReady)
	{
		return;
	}

	const float Start = FPlatformTime::Seconds();

	for (const TSharedPtr<FManagedCategory>& Ptr : ManagedCategories)
	{
		if (Ptr->HasFlag(EManagedCategoryFlags::DynamicTrait_Interval))
		{
			Ptr->Tick(DeltaTime);
		}
	}
	
	if (bRequireDiscover)
	{
		bRequireDiscover = false;
		TryDiscoverCategories();
	}
	if (bRequirePopulate)
	{
		bRequirePopulate = false;
		TryPopulateCategoryItems();
	}
	if (bRequireUpdateEngineCategories)
	{
		bRequireUpdateEngineCategories = false;
		ApplyEngineCategorySettings();
	}
	if (bRequireUpdateManagedCategories)
	{
		bRequireUpdateManagedCategories = false;
		ApplyManagedCategorySettings();
	}
	if (bRequireApplyRecentList)
	{
		bRequireApplyRecentList = false;
		ApplyRecentListSettings();
	}
	if (bRequireSettingsSave)
	{
		bRequireSettingsSave = false;
		TrySaveSettings();
	}
	if (bRequireToolbarRefresh)
	{
		bRequireToolbarRefresh = false;
		GetModuleRef().NotifyCategoriesChanged();
	}
	if (bRequireToolbarContentRefresh)
	{
		bRequireToolbarContentRefresh = false;
		GetModuleRef().TryForceContentRefresh();
	}

	const float Delta = FPlatformTime::Seconds() - Start;
	if (Delta > 5.f)
	{
		UE_LOG(LogEnhancedPalette, Warning, TEXT("Stutter for %f seconds detected"), Delta);
	}
}

void UEnhancedPaletteSubsystem::TryDiscoverCategories()
{
	FPaletteScopedTimeLogger ScopedLog(FPaletteScopedTimeLogger::START_END, TEXT("Discovering categories"), ELogVerbosity::Verbose);

	ensure(bSubsystemReady);
	ensure(!bPendingAssetLoad);

	FPlacementModeModuleAccess& Access = GetModuleRef();

	TArray<TSharedPtr<FManagedCategory>> NewDiscoveredCategories;

	// discover config and native classes, which are available early
	TryDiscoverFromConfig(NewDiscoveredCategories);

	if (GetDefault<UEnhancedPaletteSettings>()->bEnableNativeCategoryDiscovery)
	{
		TryDiscoverFromNativeScan(NewDiscoveredCategories);
	}

	if (GetDefault<UEnhancedPaletteSettings>()->bEnableAssetCategoryDiscovery)
	{
		TryDiscoverFromAssetScan(NewDiscoveredCategories);
	}

	//
	bool bChanged = false;

	// find categories that were removed in current cycle and unregister/remove them
	for (auto It = ManagedCategories.CreateIterator(); It; ++It)
	{
		TSharedPtr<FManagedCategory> Item = *It;
		if (Item->HasFlag(EManagedCategoryFlags::Type_External) && static_cast<FExternalCategory&>(*Item).bPendingKill)
		{
			continue;
		}
		if (NewDiscoveredCategories.FindByKey(Item->UniqueId) == nullptr)
		{
			// category existed but no longer in new cycle - unregister & remove
			UE_LOG(LogEnhancedPalette, Verbose, TEXT("Discover: removed outdated category %s"), *Item->UniqueId.ToString());

			Item->Unregister(this, Access);
			It.RemoveCurrent();

			bChanged = true;
		}
	}

	// find categories that were added
	for (auto& Ptr : NewDiscoveredCategories)
	{
		if (ManagedCategories.FindByKey(Ptr->UniqueId) == nullptr)
		{
			UE_LOG(LogEnhancedPalette, Verbose, TEXT("Discover: added new category %s"), *Ptr->UniqueId.ToString());

			Ptr->bDirtyContent = true; // fresh dirty by default
			Ptr->bDirtyInfo = false; // info is not dirty by default

			Ptr->Register(this, Access);
			ManagedCategories.Add(Ptr);

			bChanged = true;
		}
	}

	NewDiscoveredCategories.Reset();

	if (bChanged)
	{
		RequestToolbarRefresh();
		RequestPopulate();
	}
}

void UEnhancedPaletteSubsystem::TryDiscoverFromConfig(TArray<TSharedPtr<FManagedCategory>>& OutCategories) const
{
	FPaletteScopedTimeLogger ScopedLog(FPaletteScopedTimeLogger::END, TEXT("Searching in config"), ELogVerbosity::Verbose);

	for (const FConfigPlacementCategoryInfo& Descriptor : GetDefault<UEnhancedPaletteSettings>()->StaticCategories)
	{
		if (Descriptor.UniqueId.IsNone())
			continue;

		if (OutCategories.FindByKey(Descriptor.UniqueId) == nullptr)
		{
			auto Category = MakeShared<FConfigDrivenCategory>(Descriptor.UniqueId);
			OutCategories.Emplace(MoveTemp(Category));
		}
	}

	for (const TSoftClassPtr<UEnhancedPaletteCategory>& Ptr : GetDefault<UEnhancedPaletteSettings>()->DynamicCategories)
	{
		if (auto* AssetClass = Ptr.LoadSynchronous())
		{
			const UEnhancedPaletteCategory* CategoryCDO = AssetClass->GetDefaultObject<UEnhancedPaletteCategory>();
			if (OutCategories.FindByKey(CategoryCDO->GetCategoryUniqueId()) == nullptr)
			{
				auto Category = MakeShared<FAssetDrivenCategory>(CategoryCDO->GetCategoryUniqueId());
				Category->Source = AssetClass;

				OutCategories.Emplace(MoveTemp(Category));
			}
		}
	}
}

void UEnhancedPaletteSubsystem::TryDiscoverFromNativeScan(TArray<TSharedPtr<FManagedCategory>>& OutCategories) const
{
	FPaletteScopedTimeLogger ScopedLog(FPaletteScopedTimeLogger::END, TEXT("Searching in native"), ELogVerbosity::Verbose);

	TArray<UClass*> NativeCategories;
	GetDerivedClasses(UEnhancedPaletteCategory::StaticClass(), NativeCategories, true);

	for (UClass* AssetClass : NativeCategories)
	{
		if (AssetClass->HasAnyClassFlags(CLASS_Native) && !AssetClass->HasAnyClassFlags(CLASS_Abstract))
		{
			const UEnhancedPaletteCategory* CategoryCDO = AssetClass->GetDefaultObject<UEnhancedPaletteCategory>();
			if (OutCategories.FindByKey(CategoryCDO->GetCategoryUniqueId()) == nullptr)
			{
				auto Category = MakeShared<FAssetDrivenCategory>(CategoryCDO->GetCategoryUniqueId());
				Category->Source = AssetClass;
				OutCategories.Emplace(MoveTemp(Category));
			}
		}
	}
}

void UEnhancedPaletteSubsystem::TryDiscoverFromAssetScan(TArray<TSharedPtr<FManagedCategory>>& OutCategories) const
{
	FPaletteScopedTimeLogger ScopedLog(FPaletteScopedTimeLogger::END, TEXT("Searching in assets"), ELogVerbosity::Verbose);

	TArray<FAssetData> AllBPsAssetData;
	IAssetRegistry::Get()->GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), AllBPsAssetData, false);

	for (FAssetData& BPAssetData : AllBPsAssetData)
	{
		FString ParentClassName;
		if (!BPAssetData.GetTagValue(FBlueprintTags::NativeParentClassPath, ParentClassName))
		{
			BPAssetData.GetTagValue(FBlueprintTags::ParentClassPath, ParentClassName);
		}

		if (!ParentClassName.IsEmpty())
		{
			UObject* Outer = nullptr;
			ResolveName(Outer, ParentClassName, false, false);
			UClass* ParentClass = FindObject<UClass>(Outer, *ParentClassName);
			if (!ParentClass || !ParentClass->IsChildOf(UEnhancedPaletteCategory::StaticClass()))
			{
				continue;
			}
		}

		UBlueprint* Blueprint = ExactCast<UBlueprint>(BPAssetData.GetAsset());
		if (UClass* AssetClass = Blueprint ? Blueprint->GeneratedClass : nullptr)
		{
			const UEnhancedPaletteCategory* CategoryCDO = AssetClass->GetDefaultObject<UEnhancedPaletteCategory>();
			if (OutCategories.FindByKey(CategoryCDO->GetCategoryUniqueId()) == nullptr)
			{
				auto Category = MakeShared<FAssetDrivenCategory>(CategoryCDO->GetCategoryUniqueId());
				Category->Source = AssetClass;
				//Category->Instance = NewObject<UEnhancedPaletteCategory>(GetTransientPackage(), AssetClass, NAME_None, RF_Transient);
				OutCategories.Emplace(MoveTemp(Category));
			}
		}
	}
}

void UEnhancedPaletteSubsystem::TryPopulateCategoryItems()
{
	FPaletteScopedTimeLogger ScopedLog(FPaletteScopedTimeLogger::START_END, TEXT("Populating category items"), ELogVerbosity::Verbose);

	FPlacementModeModuleAccess& Access = GetModuleRef();

	//Access->RegenerateItemsForCategory(FBuiltInPlacementCategories::RecentlyPlaced());
	//Access->RegenerateItemsForCategory(FBuiltInPlacementCategories::Volumes());
	//Access->RegenerateItemsForCategory(FBuiltInPlacementCategories::AllClasses());
	//Access->RegenerateItemsForCategory(FBuiltInPlacementCategories::Favorites());
	bool bChanged = false;

	for (const TSharedPtr<FManagedCategory>& Ptr : ManagedCategories)
	{
		if (Ptr->bDirtyContent)
		{
			FPaletteScopedTimeLogger ScopeForCategory(FPaletteScopedTimeLogger::START_END, Ptr->UniqueId.ToString(), ELogVerbosity::Verbose);
			
			Ptr->bDirtyContent = false;
			bChanged = true;

			// purge all existing registrations within category
			for (const FPlacementModeID& ManagedId : Ptr->ManagedIds)
			{
				Access->UnregisterPlaceableItem(ManagedId);
			}
			Ptr->ManagedIds.Empty();

			TArray<TInstancedStruct<FConfigPlaceableItem>> Result;
			Ptr->GatherPlaceableItems(this, Result);

			TArray<FName> KnownInternalNames;
			KnownInternalNames.Reserve(Result.Num());

			for (const TInstancedStruct<FConfigPlaceableItem>& ConfigItem : Result)
			{
				if (!ConfigItem.IsValid() || !ConfigItem.Get<FConfigPlaceableItem>().IsValidData())
					continue;

				if (TSharedPtr<FPlaceableItem> Item = ConfigItem.Get<FConfigPlaceableItem>().MakeItem())
				{
					UE_LOG(LogEnhancedPalette, Verbose, TEXT("Register Placement Item: Category=%s Name=%s Factory=%s ObjectData=%s"),
						*Ptr->UniqueId.ToString(),
						*Item->GetNativeFName().ToString(),
						*GetPathNameSafe(Item->AssetFactory.GetObject()),
						*Item->AssetData.ToSoftObjectPath().ToString()
					);

					if (KnownInternalNames.Contains(Item->GetNativeFName()))
					{
						UE_LOG(LogEnhancedPalette, Warning, TEXT("Duplicating native name found [Category=%s Name=%s] it may affect favorites list"),
							*Ptr->UniqueId.ToString(),
							*Item->GetNativeFName().ToString());
						// continue;
					}

					TOptional<FPlacementModeID> Id = Access->RegisterPlaceableItem(Ptr->UniqueId, Item.ToSharedRef());
					if (Id.IsSet())
					{
						KnownInternalNames.Add(Item->GetNativeFName());
						Ptr->ManagedIds.Add(Id.GetValue());
					}
					else
					{
						UE_LOG(LogEnhancedPalette, Warning, TEXT("Register Placement Item: Failed"));
					}
					
				}
			}

			Access.NotifyCategoryRefreshed(Ptr->UniqueId);
		}
	}

	if (bChanged)
	{
		RequestToolbarRefresh();
		RequestToolbarContentRefresh();
	}
}

bool UEnhancedPaletteSubsystem::CreateExternalCategory(const FStaticPlacementCategoryInfo& CreationInfo)
{
	if (FindManagedCategory(CreationInfo.UniqueId) != nullptr)
	{
		UE_LOG(LogEnhancedPalette, Warning, TEXT("Failed to create external category: already registered"))
		return false;
	}

	auto Category = MakeShared<FExternalCategory>(CreationInfo.UniqueId);
	Category->Data = CreationInfo;
	ManagedCategories.Add(MoveTemp(Category));
	
	// would need update as new discovery was made
	RequestDiscover();
	return true;
}

bool UEnhancedPaletteSubsystem::RemoveExternalCategory(const FName& UniqueId)
{
	auto Found = FindManagedCategory(UniqueId);
	if (Found && Found->HasFlag(EManagedCategoryFlags::Type_External))
	{
		static_cast<FExternalCategory&>(*Found).bPendingKill = true;
		RequestDiscover();
		return true;
	}
	return false;
}

bool UEnhancedPaletteSubsystem::AddExternalCategoryItem(const FName& UniqueId, TInstancedStruct<FConfigPlaceableItem> Item)
{
	auto Found = FindManagedCategory(UniqueId);
	if (Found && Found->HasFlag(EManagedCategoryFlags::Type_External))
	{
		static_cast<FExternalCategory&>(*Found).Data.Items.Add(MoveTemp(Item));
		RequestPopulate();
		return true;
	}
	return false;
}

void UEnhancedPaletteSubsystem::Deinitialize()
{
	UE_LOG(LogEnhancedPalette, Verbose, TEXT("DeInitializing subsystem"));

	bSubsystemReady = false;

	ExternalChangeTracker->UnregisterTrackers(this);
	ExternalChangeTracker.Reset();

	FModuleManager::Get().OnModulesChanged().RemoveAll(this);

	{
		FPlacementModeModuleAccess& ModuleRef = GetModuleRef();
		for (const TSharedPtr<FManagedCategory>& Ptr : ManagedCategories)
		{
			Ptr->Unregister(this, ModuleRef);
		}
	}

	ManagedCategories.Empty();
	ModuleAccessPrivate.Reset();
}

void UEnhancedPaletteSubsystem::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(InThis, Collector);

	UEnhancedPaletteSubsystem* Self = CastChecked<UEnhancedPaletteSubsystem>(InThis);
	for (const TSharedPtr<FManagedCategory>& Ptr : Self->ManagedCategories)
	{
		Ptr->AddReferencedObjects(Collector, Self);
	}
}

void UEnhancedPaletteSubsystem::ApplyEngineCategorySettings()
{
	FPaletteScopedTimeLogger ScopedLog(FPaletteScopedTimeLogger::END, TEXT("ApplyEngineCategorySettings"), ELogVerbosity::Verbose);

	auto& ModuleRef = GetModuleRef();
	auto* Settings = GetMutableDefault<UEnhancedPaletteSettings>();

	// Phase 1: apply sort order changes

	for (const FStandardPlacementCategoryInfo& Pair : Settings->EngineCategories)
	{
		if (auto* Category = const_cast<FPlacementCategoryInfo*>(ModuleRef->GetRegisteredPlacementCategory(Pair.UniqueId)))
		{
			Category->SortOrder = Pair.Order;
			// todo: here can possibly change data in engine categories
		}
	}

	// Phase 2: apply visibility changes based on permission list

	const FName OWNER_ID = UEnhancedPaletteSubsystem::StaticClass()->GetFName();
	
	auto PermListAccess = StaticCastSharedRef<FPermissionListAccess>(ModuleRef->GetCategoryPermissionList());

	{
		TGuardValue<bool> Guard(PermListAccess->bSuppressOnFilterChanged, true);
		PermListAccess->UnregisterOwner(OWNER_ID);
		for (const FName& KnownCategory : Settings->CachedKnownCategories)
		{
			PermListAccess->RemoveDenyListItem(OWNER_ID, KnownCategory);
		}
		for (const FStandardPlacementCategoryInfo& Pair : Settings->EngineCategories)
		{
			if (!Pair.bVisible)
			{
				PermListAccess->AddDenyListItem(OWNER_ID, Pair.UniqueId);
			}
		}
	}

	PermListAccess->NotifyChanged();
	
	RequestToolbarRefresh();
}

void UEnhancedPaletteSubsystem::ApplyManagedCategorySettings()
{
	FPaletteScopedTimeLogger ScopedLog(FPaletteScopedTimeLogger::END, TEXT("ApplyManagedCategorySettings"), ELogVerbosity::Verbose);

	auto& Access = GetModuleRef();
	for (const TSharedPtr<FManagedCategory>& Ptr : ManagedCategories)
	{
		if (Ptr->bDirtyInfo)
		{
			Ptr->bDirtyInfo = false;
			if (Ptr->UpdateRegistration(this, Access))
			{
				Access.NotifyCategoryRefreshed(Ptr->UniqueId);
			}
		}
	}
}

void UEnhancedPaletteSubsystem::ApplyRecentListSettings()
{
	FPaletteScopedTimeLogger ScopedLog(FPaletteScopedTimeLogger::END, TEXT("ApplyRecentListSettings"), ELogVerbosity::Verbose);
	// set recent list w/notify
	GetModuleRef().SetRecentList(GetDefault<UEnhancedPaletteSettings>()->RecentlyPlaced);
}

void UEnhancedPaletteSubsystem::OnSettingsPanelSelected()
{
	UE_LOG(LogEnhancedPalette, Verbose, TEXT("OnSettingsPanelSelected"));

	auto& ModuleRef = GetModuleRef();
	auto* Settings = GetMutableDefault<UEnhancedPaletteSettings>();

	const TArray<FStandardPlacementCategoryInfo> LastCycleData = Settings->EngineCategories;
	Settings->EngineCategories.Empty();

	// import visibility and order switches
	for (const FName& UniqueHandle : ModuleRef.GetRegisteredCategoryNames())
	{
		Settings->CachedKnownCategories.AddUnique(UniqueHandle);
		// display only categories not managed by plugin (registered in engine)
		const bool bIsManaged = FindManagedCategory(UniqueHandle) != nullptr;

		if (bIsManaged)
		{ // either rename or something else
			Settings->EngineCategories.RemoveAll([UniqueHandle](const FStandardPlacementCategoryInfo& Info) { return Info.UniqueId == UniqueHandle; });
			continue;
		}
		
		if (const FStandardPlacementCategoryInfo* LastCycleInfo = LastCycleData.FindByKey(UniqueHandle))
		{ // pick last cycle data
			Settings->EngineCategories.Emplace(*LastCycleInfo);
			continue;
		}
		
		if (const FPlacementCategoryInfo* CategoryInfo = ModuleRef->GetRegisteredPlacementCategory(UniqueHandle))
		{ // new entry
			FStandardPlacementCategoryInfo Info;
			Info.UniqueId = UniqueHandle;
			Info.Order = CategoryInfo->SortOrder;
			Info.bVisible = true;
			Settings->EngineCategories.Emplace(Info);
		}
	}
	// keep the list sorted to values of their sort orders
	// TBD: possibly utilize reordering of array items?
	Algo::Sort(Settings->EngineCategories, [](const FStandardPlacementCategoryInfo& Left, const FStandardPlacementCategoryInfo& Right)
	{
		return Left.Order < Right.Order;
	});

	// import recent list to settings panel 
	Settings->RecentlyPlaced.Empty();
	for (const FActorPlacementInfo& Element : ModuleRef->GetRecentlyPlaced())
	{
		FConfigActorPlacementInfo Desc;
		Desc.Factory = Element.Factory;
		Desc.ObjectPath = Element.ObjectPath;

		{
			FStringBuilderBase SB;
			if (!Desc.Factory.IsEmpty())
			{
				int32 IndexOfLastSlash = INDEX_NONE;
				Desc.Factory.FindLastChar(TEXT('.'), IndexOfLastSlash);
				SB.Append(TEXT("F="));
				SB.Append(Desc.Factory.Mid(IndexOfLastSlash + 1));
				SB.Append(TEXT(" "));
			}
			if (!Desc.ObjectPath.IsEmpty())
			{
				SB.Append(TEXT("O="));
				SB.Append(Desc.ObjectPath);
			}
			Desc.DisplayLabel = SB.ToString();
		}

		Settings->RecentlyPlaced.Add(MoveTemp(Desc));
	}
}

void UEnhancedPaletteSubsystem::OnSettingsPanelModified(UObject* Obj, FPropertyChangedEvent& Evt)
{
	if (!bSubsystemReady) return;

	const UEnhancedPaletteSettings* Settings = GetDefault<UEnhancedPaletteSettings>();
	ensure(Obj == Settings);

	const FName MemberPropName = Evt.GetMemberPropertyName();
	const FName ChangedPropName = Evt.GetPropertyName();

	UE_LOG(LogEnhancedPalette, Verbose, TEXT("OnSettingsChanged %s :: %s"), *MemberPropName.ToString(), *ChangedPropName.ToString());
	
	auto IsMemberOf = [](const FProperty* InProperty, const UStruct* InStruct) -> bool
	{
		return InProperty && InProperty->GetOwnerStruct()->IsChildOf(InStruct);
	};
	
	const bool bSettingsChange = IsMemberOf(Evt.Property, UEnhancedPaletteSettings::StaticClass());
	const bool bCategoryChange = IsMemberOf(Evt.Property, FConfigPlacementCategoryInfo::StaticStruct());
	const bool bItemChange = IsMemberOf(Evt.Property, FConfigPlaceableItem::StaticStruct());

	if (IsMemberOf(Evt.MemberProperty, UEnhancedPaletteSettings::StaticClass()))
	{
		if (MemberPropName == GET_MEMBER_NAME_CHECKED(UEnhancedPaletteSettings, RecentlyPlaced))
		{
			RequestRecentListSave();
		}
		else if (MemberPropName == GET_MEMBER_NAME_CHECKED(UEnhancedPaletteSettings, EngineCategories))
		{
			RequestUpdateCategoryData();
		}
		else if (MemberPropName == GET_MEMBER_NAME_CHECKED(UEnhancedPaletteSettings, DynamicCategories))
		{
			RequestDiscover();
		}
		else if (MemberPropName == GET_MEMBER_NAME_CHECKED(UEnhancedPaletteSettings, StaticCategories))
		{
			RequestSettingsSave();
		}
	}

	if (bCategoryChange || bItemChange)
	{
		RequestSettingsSave();
		RequestDiscover();
		// since it is difficult which category data was modified - mark all static categories as dirty
		for (const FStaticPlacementCategoryInfo& Info : Settings->StaticCategories)
		{
			MarkCategoryDirty(Info.UniqueId, EManagedCategoryDirtyFlags::All);
		}
	}
}

void UEnhancedPaletteSubsystem::OnSettingsPanelCommand(FName InCommand)
{
	UE_LOG(LogEnhancedPalette, Verbose, TEXT("OnSettingsPanelCommand %s"), *InCommand.ToString());

	if (InCommand == SettingsCommand::DiscoverCategories)
	{
		RequestDiscover();
	}
	if (InCommand == SettingsCommand::PopulateCategories)
	{
		MarkCategoryDirty(EManagedCategoryFlags::Type_Any, EManagedCategoryDirtyFlags::Content);
		RequestPopulate();
		RequestToolbarContentRefresh();
	}
	if (InCommand == SettingsCommand::UpdateCategores)
	{
		MarkCategoryDirty(EManagedCategoryFlags::Type_Any, EManagedCategoryDirtyFlags::Info);
		RequestUpdateCategoryData();
	}
	if (InCommand == SettingsCommand::UpdateToolbar)
	{
		RequestToolbarRefresh();
		RequestToolbarContentRefresh();
	}
	if (InCommand == SettingsCommand::ClearRecent)
	{
		GetModuleRef().SetRecentList({});
		RequestToolbarContentRefresh();
	}
}

void UEnhancedPaletteSubsystem::TrySaveSettings()
{
	// BUG: force save settings section due to InstancedStructs save bug
	if (auto Ptr = SettingsSectionPtr.Pin())
	{
		Ptr->Save();
	}
}

void UEnhancedPaletteSubsystem::OnPlacementModeCategoryListChanged()
{
	UE_LOG(LogEnhancedPalette, Verbose, TEXT("OnPlacementModeCategoryListChanged"));
	if (bSubsystemReady)
	{
		// Try import data
    	OnSettingsPanelSelected();
	}
}

void UEnhancedPaletteSubsystem::OnPlaceableItemFilteringChanged()
{
	UE_LOG(LogEnhancedPalette, Verbose, TEXT("OnPlaceableItemFilteringChanged"));
	if (bSubsystemReady)
	{
		// BUG: The content doesn't actually change, no need to dirty but force refresh UI as it is broken
    	// And fix for 5.5+
		// MarkCategoryDirty(EManagedCategoryFlags::Type_Any, EManagedCategoryDirtyFlags::Content);
    	RequestToolbarContentRefresh();
	}
}

void UEnhancedPaletteSubsystem::OnAllPlaceableAssetsChanged()
{
	UE_LOG(LogEnhancedPalette, Verbose, TEXT("OnAllPlaceableAssetsChanged"));
	if (bSubsystemReady)
	{
		// BUG: Placeable assets were modified, which means dynamic asset categories may have new content
		// And fix for 5.5+ again
        MarkCategoryDirty(EManagedCategoryFlags::DynamicTrait_Asset, EManagedCategoryDirtyFlags::Content);
        RequestToolbarContentRefresh();
	}
}

void UEnhancedPaletteSubsystem::OnRecentlyPlacedChanged(const TArray<FActorPlacementInfo>& List)
{
	UE_LOG(LogEnhancedPalette, Verbose, TEXT("OnRecentlyPlacedChanged"));
	for (const FActorPlacementInfo& Info : List)
	{
		UE_LOG(LogEnhancedPalette, Verbose, TEXT("%s;%s"), *Info.ObjectPath, *Info.Factory);
	}
}

void UEnhancedPaletteSubsystem::OnCategoryBlueprintModified(class UBlueprint*, FName ID)
{
	UE_LOG(LogEnhancedPalette, Verbose, TEXT("OnCategoryBlueprintModified %s "), *ID.ToString());
	if (bSubsystemReady)
	{
		MarkCategoryDirty(ID, EManagedCategoryDirtyFlags::All);
	}
}

void UEnhancedPaletteSubsystem::OnCategoryObjectModified(UObject* InObj, struct FPropertyChangedEvent&, FName InCategory)
{
	UE_LOG(LogEnhancedPalette, Verbose, TEXT("OnCategoryObjectModified %s "), *InCategory.ToString());

	if (bSubsystemReady)
	{
		if (auto Category = FindManagedCategory(InCategory))
		{
			ensure(Category->HasFlag(EManagedCategoryFlags::Type_Asset));
			if (InObj == static_cast<FAssetDrivenCategory*>(Category.Get())->Instance
				|| InObj == static_cast<FAssetDrivenCategory*>(Category.Get())->InstanceDefault)
			{
				MarkCategoryDirty(InCategory, EManagedCategoryDirtyFlags::All);
			}
		}
	}
}
