// Copyright 2025, Aquanox.

#include "EnhancedPaletteSubsystemPrivate.h"

#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EnhancedPaletteSubsystem.h"
#include "EnhancedPaletteGlobals.h"
#include "EnhancedPaletteCategory.h"
#include "PlacementModeModuleAccess.h"

FManagedCategory::FManagedCategory(FName InUniqueId, EManagedCategoryFlags InBase): UniqueId(InUniqueId), Flags(InBase)
{
}

void FManagedCategory::AddReferencedObjects(FReferenceCollector& Collector, UObject* Owner)
{
}

void FManagedCategory::Tick(float DeltaTime)
{
}

FConfigDrivenCategory::FConfigDrivenCategory(FName InUniqueId): FManagedCategory(InUniqueId, EManagedCategoryFlags::Type_Config)
{
}

FConfigDrivenCategory::FConfigDrivenCategory(FName InUniqueId, EManagedCategoryFlags InBase) : FManagedCategory(InUniqueId, InBase)
{
}

EManagedCategoryFlags FConfigDrivenCategory::GetCategoryTypeFlag() const
{
	return EManagedCategoryFlags::Type_Config;
}

const FStaticPlacementCategoryInfo* FConfigDrivenCategory::GetConfig() const
{
	return GetDefault<UEnhancedPaletteSettings>()->StaticCategories.FindByKey(UniqueId);
}

void FConfigDrivenCategory::Register(UEnhancedPaletteSubsystem* Owner, FPlacementModeModuleAccess& Access)
{
	if (!bRegistered)
	{
		if (const FStaticPlacementCategoryInfo* CategoryInfo = GetConfig())
		{
			ensure(CategoryInfo->UniqueId == UniqueId);
			FPlacementCategoryInfo Reg(
				CategoryInfo->DisplayName,
				CategoryInfo->DisplayIcon.GetSlateIcon(),
				CategoryInfo->UniqueId,
				CategoryInfo->TagMetaData,
				CategoryInfo->SortOrder,
				CategoryInfo->bSortable
			);
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 5, 0)
			Reg.ShortDisplayName = CategoryInfo->ShortDisplayName.IsEmptyOrWhitespace() ? CategoryInfo->DisplayName : CategoryInfo->ShortDisplayName;
#endif
			bool bWasRegistered = Access->RegisterPlacementCategory(Reg);
			if (bWasRegistered)
			{
				// tbd
				bRegistered = true;
			}
			else
			{
				UE_LOG(LogEnhancedPalette, Error, TEXT("Failed register category %s"), *UniqueId.ToString());
			}
		}
	}
}

bool FConfigDrivenCategory::UpdateRegistration(UEnhancedPaletteSubsystem* Owner, FPlacementModeModuleAccess& Access)
{
	if (bRegistered)
	{
		const FStaticPlacementCategoryInfo* CategoryInfo = GetConfig();
		FPlacementCategoryInfo* Category = const_cast<FPlacementCategoryInfo*>(Access->GetRegisteredPlacementCategory(UniqueId));
		if (CategoryInfo && Category)
		{
			Category->DisplayName = CategoryInfo->DisplayName;
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 5, 0)
			Category->ShortDisplayName = CategoryInfo->ShortDisplayName.IsEmptyOrWhitespace() ? CategoryInfo->DisplayName : CategoryInfo->ShortDisplayName;
#endif
			Category->SortOrder = CategoryInfo->SortOrder;
			Category->bSortable = CategoryInfo->bSortable;
			Category->TagMetaData = CategoryInfo->TagMetaData;
			Category->DisplayIcon = CategoryInfo->DisplayIcon.GetSlateIcon();
			return true;
		}
	}
	return false;
}

void FConfigDrivenCategory::Unregister(UEnhancedPaletteSubsystem* Owner, FPlacementModeModuleAccess& Access)
{
	if (bRegistered)
	{
		for (const FPlacementModeID& ManagedId : ManagedIds)
		{
			Access->UnregisterPlaceableItem(ManagedId);
		}
		Access->UnregisterPlacementCategory(UniqueId);

		ManagedIds.Empty();
		bRegistered = false;
	}
}

void FConfigDrivenCategory::GatherPlaceableItems(UEnhancedPaletteSubsystem* Owner, TArray<TInstancedStruct<FConfigPlaceableItem>>& Out)
{
	if (const FStaticPlacementCategoryInfo* CategoryInfo = GetConfig())
	{
		Out.Reserve(CategoryInfo->Items.Num());
		Out.Append(CategoryInfo->Items);
	}
}

FAssetDrivenCategory::FAssetDrivenCategory(FName InUniqueId): FManagedCategory(InUniqueId, EManagedCategoryFlags::Type_Asset)
{
}

EManagedCategoryFlags FAssetDrivenCategory::GetCategoryTypeFlag() const
{
	return EManagedCategoryFlags::Type_Asset;
}

void FAssetDrivenCategory::Register(UEnhancedPaletteSubsystem* Owner, FPlacementModeModuleAccess& Access)
{
	if (!bRegistered)
	{
		UClass* CategoryClass = Source.LoadSynchronous();

		const UEnhancedPaletteCategory* DefaultInstance = GetDefault<UEnhancedPaletteCategory>(CategoryClass);
		ensureMsgf(DefaultInstance->GetCategoryUniqueId() == UniqueId, TEXT("GetCategoryUniqueId returned different identifier than initially set"));
		InstanceDefault = const_cast<UEnhancedPaletteCategory*>(DefaultInstance);

		FPlacementCategoryInfo Reg(
			DefaultInstance->GetDisplayName(),
			DefaultInstance->GetDisplayIcon(),
			DefaultInstance->GetCategoryUniqueId(),
			DefaultInstance->GetTagMetaData(),
			DefaultInstance->GetSortOrder(),
			DefaultInstance->IsSortable()
		);
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 5, 0)
		Reg.ShortDisplayName = DefaultInstance->GetShortDisplayName();
#endif
		bool bWasRegistered = Access->RegisterPlacementCategory(Reg);
		if (bWasRegistered)
		{
			FString InstanceName = FString::Printf(TEXT("%s_%s"), *CategoryClass->GetName(), *UniqueId.ToString());
			Instance = NewObject<UEnhancedPaletteCategory>(GetTransientPackage(), CategoryClass, *InstanceName, RF_Transient);
			Instance->Initialize(Owner);

			if (ExactCast<UBlueprintGeneratedClass>(CategoryClass))
			{
				auto Blueprint = CastChecked<UBlueprint>(CategoryClass->ClassGeneratedBy);
				//HandleOnChanged = Blueprint->OnChanged().AddUObject(Owner, &UEnhancedPaletteSubsystem::OnCategoryBlueprintModified, UniqueId);
				HandleOnCompiled = Blueprint->OnCompiled().AddUObject(Owner, &UEnhancedPaletteSubsystem::OnCategoryBlueprintModified, UniqueId);
				//HandleOnModified = FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(Owner, &OnCategoryObjectModified, this);
			}

			UpdateTraits(Owner, DefaultInstance);

			bRegistered = true;
		}
		else
		{
			UE_LOG(LogEnhancedPalette, Error, TEXT("Failed register category %s"), *UniqueId.ToString());
		}
	}
}

void FAssetDrivenCategory::Unregister(UEnhancedPaletteSubsystem* Owner, FPlacementModeModuleAccess& Access)
{
	if (bRegistered)
	{
		UClass* CategoryClass = Source.Get();
		if (CategoryClass && ExactCast<UBlueprintGeneratedClass>(CategoryClass))
		{
			auto Blueprint = CastChecked<UBlueprint>(CategoryClass->ClassGeneratedBy);
			//Blueprint->OnChanged().Remove(HandleOnChanged);
			Blueprint->OnCompiled().Remove(HandleOnCompiled);
			//FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(HandleOnModified);
		}

		if (IsValid(Instance))
		{
			Instance->MarkAsGarbage();
		}

		Instance = nullptr;
		InstanceDefault = nullptr;

		for (const FPlacementModeID& ManagedId : ManagedIds)
		{
			Access->UnregisterPlaceableItem(ManagedId);
		}
		ManagedIds.Empty();

		Access->UnregisterPlacementCategory(UniqueId);
		bRegistered = false;
	}
}

bool FAssetDrivenCategory::UpdateRegistration(UEnhancedPaletteSubsystem* Owner, FPlacementModeModuleAccess& Access)
{
	if (bRegistered)
	{
		auto DefaultInstance = GetDefault<UEnhancedPaletteCategory>(Source.LoadSynchronous());
		ensureMsgf(DefaultInstance->GetCategoryUniqueId() == UniqueId, TEXT("GetCategoryUniqueId returned different identifier than initially set"));
		InstanceDefault = const_cast<UEnhancedPaletteCategory*>(DefaultInstance);

		UpdateTraits(Owner, DefaultInstance);

		if (auto* Category = const_cast<FPlacementCategoryInfo*>(Access->GetRegisteredPlacementCategory(UniqueId)))
		{
			Category->DisplayName = DefaultInstance->GetDisplayName();
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 5, 0)
			Category->ShortDisplayName = DefaultInstance->GetShortDisplayName();
#endif
			Category->SortOrder = DefaultInstance->GetSortOrder();
			Category->bSortable = DefaultInstance->IsSortable();
			Category->TagMetaData = DefaultInstance->GetTagMetaData();
			Category->DisplayIcon = DefaultInstance->GetDisplayIcon();
			return true;
		}
	}
	return false;
}

void FAssetDrivenCategory::UpdateTraits(UEnhancedPaletteSubsystem* Owner, const UEnhancedPaletteCategory* InCategory)
{
	if (InCategory->bTickable)
	{
		SetFlag(EManagedCategoryFlags::DynamicTrait_Interval);
	}
	else
	{
		UnsetFlag(EManagedCategoryFlags::DynamicTrait_Interval);
	}

	if (InCategory->bTrackingBlueprintChanges)
	{
		SetFlag(EManagedCategoryFlags::DynamicTrait_Blueprint);
	}
	else
	{
		UnsetFlag(EManagedCategoryFlags::DynamicTrait_Blueprint);
	}

	if (InCategory->bTrackingAssetChanges)
    {
	    SetFlag(EManagedCategoryFlags::DynamicTrait_Asset);
    }
    else
    {
	    UnsetFlag(EManagedCategoryFlags::DynamicTrait_Asset);
    }

	if (InCategory->bTrackingWorldChanges)
    {
	    SetFlag(EManagedCategoryFlags::DynamicTrait_World);
    }
    else
    {
	    UnsetFlag(EManagedCategoryFlags::DynamicTrait_World);
    }
}

void FAssetDrivenCategory::GatherPlaceableItems(UEnhancedPaletteSubsystem* Owner, TArray<TInstancedStruct<FConfigPlaceableItem>>& Out)
{
	if (bRegistered && ensure(IsValid(Instance)))
	{
		Out.Reserve(32);
		Instance->GatherItems(Out);
	}
}

void FAssetDrivenCategory::Tick(float DeltaTime)
{
	if (bRegistered && ensure(IsValid(Instance)))
	{
		Instance->Tick(DeltaTime);
	}
}

FExternalCategory::FExternalCategory(FName InUniqueId): FConfigDrivenCategory(InUniqueId, EManagedCategoryFlags::Type_External)
{
}

EManagedCategoryFlags FExternalCategory::GetCategoryTypeFlag() const
{
	return EManagedCategoryFlags::Type_External;
}

void FAssetDrivenCategory::AddReferencedObjects(FReferenceCollector& Collector, UObject* Owner)
{
	Collector.AddReferencedObject(Instance, Owner);
	Collector.AddReferencedObject(InstanceDefault, Owner);
}

FManagedCategoryChangeTracker::~FManagedCategoryChangeTracker()
{
	UnregisterTrackers(nullptr);
}

bool FManagedCategoryChangeTracker::IsTrackingEnabled()
{
	return GetDefault<UEnhancedPaletteSettings>()->bEnableChangeTrackingFeatures;
}

void FManagedCategoryChangeTracker::RegisterTrackers(UEnhancedPaletteSubsystem* Owner)
{
	// BLUEPRINT

	GEditor->OnBlueprintCompiled().AddSPLambda(this, [Owner]()
	{
		if (IsTrackingEnabled())
		{
			UE_LOG(LogEnhancedPalette, Verbose, TEXT("Tracking::OnBlueprintRecompiled"));
			Owner->MarkCategoryDirty(EManagedCategoryFlags::DynamicTrait_Blueprint, EManagedCategoryDirtyFlags::Content);
		}
	});

	// ASSET

	IAssetRegistry& Registry = IAssetRegistry::GetChecked();
	Registry.OnAssetAdded().AddSPLambda(this, [Owner](const FAssetData& AssetData)
	{
		if (IsTrackingEnabled())
		{
			UE_LOG(LogEnhancedPalette, Verbose, TEXT("Tracking::OnAssetAdded"));
			Owner->MarkCategoryDirty(EManagedCategoryFlags::DynamicTrait_Asset, EManagedCategoryDirtyFlags::Content);
		}
	});
	Registry.OnAssetRenamed().AddSPLambda(this, [Owner](const FAssetData& AssetData, const FString& String)
	{
		if (IsTrackingEnabled())
		{
			UE_LOG(LogEnhancedPalette, Verbose, TEXT("Tracking::OnAssetRenamed"));
			Owner->MarkCategoryDirty(EManagedCategoryFlags::DynamicTrait_Asset, EManagedCategoryDirtyFlags::Content);
		}
	});
	Registry.OnAssetRemoved().AddSPLambda(this, [Owner](const FAssetData& AssetData)
	{
		if (IsTrackingEnabled())
		{
			UE_LOG(LogEnhancedPalette, Verbose, TEXT("Tracking::OnAssetRemoved"));
			Owner->MarkCategoryDirty(EManagedCategoryFlags::DynamicTrait_Asset, EManagedCategoryDirtyFlags::Content);
		}
	});
	FEditorDelegates::OnAssetsDeleted.AddSPLambda(this, [Owner](const TArray<UClass*>& /*DeletedAssetClasses*/)
	{
		if (IsTrackingEnabled())
		{
			UE_LOG(LogEnhancedPalette, Verbose, TEXT("Tracking::OnAssetsDeleted"));
			Owner->MarkCategoryDirty(EManagedCategoryFlags::DynamicTrait_Asset, EManagedCategoryDirtyFlags::Content);
		}
	});

	// WORLD

	FEditorDelegates::OnNewActorsDropped.AddSPLambda(this, [Owner](const TArray<UObject*>&, const TArray<AActor*>&)
	{
		if (IsTrackingEnabled())
		{
			UE_LOG(LogEnhancedPalette, Verbose, TEXT("Tracking::OnNewActorsDropped"));
			Owner->MarkCategoryDirty(EManagedCategoryFlags::DynamicTrait_World, EManagedCategoryDirtyFlags::Content);
		}
	});
	FEditorDelegates::OnNewActorsPlaced.AddSPLambda(this, [Owner](UObject*, const TArray<AActor*>&)
	{
		if (IsTrackingEnabled())
		{
			UE_LOG(LogEnhancedPalette, Verbose, TEXT("Tracking::OnNewActorsPlaced"));
			Owner->MarkCategoryDirty(EManagedCategoryFlags::DynamicTrait_World, EManagedCategoryDirtyFlags::Content);
		}
	});
	FEditorDelegates::OnDeleteActorsEnd.AddSPLambda(this, [Owner]()
	{
		if (IsTrackingEnabled())
		{
			UE_LOG(LogEnhancedPalette, Verbose, TEXT("Tracking::OnDeleteActorsEnd"));
			Owner->MarkCategoryDirty(EManagedCategoryFlags::DynamicTrait_World, EManagedCategoryDirtyFlags::Content);
		}
	});
	FEditorDelegates::OnMapOpened.AddSPLambda(this, [Owner](const FString& /* Filename */, bool /*bAsTemplate*/)
	{
		if (IsTrackingEnabled())
		{
			UE_LOG(LogEnhancedPalette, Verbose, TEXT("Tracking::OnMapOpened"));
			Owner->MarkCategoryDirty(EManagedCategoryFlags::DynamicTrait_World, EManagedCategoryDirtyFlags::Content);
		}
	});
	FEditorDelegates::OnMapLoad.AddSPLambda(this, [Owner](const FString& /* Filename */, FCanLoadMap& /*OutCanLoadMap*/)
	{
		if (IsTrackingEnabled())
		{
			UE_LOG(LogEnhancedPalette, Verbose, TEXT("Tracking::OnMapLoad"));
			Owner->MarkCategoryDirty(EManagedCategoryFlags::DynamicTrait_World, EManagedCategoryDirtyFlags::Content);
		}
	});
}

void FManagedCategoryChangeTracker::UnregisterTrackers(UEnhancedPaletteSubsystem* Owner)
{
	// Trait Blueprint
	GEditor->OnBlueprintReinstanced().RemoveAll(this);

	// Trait Asset
	IAssetRegistry& Registry = IAssetRegistry::GetChecked();
	Registry.OnAssetAdded().RemoveAll(this);
	Registry.OnAssetRenamed().RemoveAll(this);
	Registry.OnAssetRemoved().RemoveAll(this);
	FEditorDelegates::OnAssetsDeleted.RemoveAll(this);

	// Trait World
	FEditorDelegates::OnNewActorsDropped.RemoveAll(this);
	FEditorDelegates::OnNewActorsPlaced.RemoveAll(this);
	FEditorDelegates::OnDeleteActorsEnd.RemoveAll(this);
	FEditorDelegates::OnMapLoad.RemoveAll(this);
	FEditorDelegates::OnMapOpened.RemoveAll(this);
}
