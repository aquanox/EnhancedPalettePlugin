// Copyright 2025, Aquanox.

#include "ExampleNativeCategory.h"
#include "Editor.h"
#include "ActorFactories/ActorFactoryBasicShape.h"
#include "ActorFactories/ActorFactoryCharacter.h"
#include "ActorFactories/ActorFactoryPawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExampleNativeCategory)

namespace FeatureSwitches
{
	constexpr bool bRegisterPeriodicUpdate = false;
	constexpr bool bRegisterDelegates = false;
}

UExampleNativeCategory::UExampleNativeCategory()
{
	DisplayName = INVTEXT("Native");
	ShortDisplayName = INVTEXT("Native");
	DisplayIcon = FSimpleIconSelector("EditorStyle", "ContentPalette.ShowParticles");

	// can register for periodic updates
	if (FeatureSwitches::bRegisterPeriodicUpdate)
	{
		bTickable = true;
		TickInterval = 10.f;
	}
}

void UExampleNativeCategory::NativeInitialize()
{
	// can register to delegates that will trigger content updates 
	if (FeatureSwitches::bRegisterDelegates)
	{
		FEditorDelegates::OnAssetsDeleted.AddWeakLambda(this, [this](const TArray<UClass*>& InArray)
		{
			NotifyContentChanged();
		});
	}
}

void UExampleNativeCategory::NativeTick()
{
	constexpr bool bSomeCondition = false;
	// can register for periodic updates and request category update.
	if (FeatureSwitches::bRegisterPeriodicUpdate && bSomeCondition)
	{
		NotifyContentChanged();
	}
}

void UExampleNativeCategory::NativeGatherItems()
{
	// Preferred - construct FPlaceableItem directly with desired params
	// More Examples can be found in PlacementModeModule.cpp FPlacementModeModule::StartupModule
	
	// PlaceableItem arg descriptor
	AddPlaceableItem<FPlaceableItem>(*UActorFactoryEmptyActor::StaticClass(), TOptional<int32>());
	
	// PlaceableItem descriptor
	AddPlaceableItemPtr(MakeShared<FPlaceableItem>(*UActorFactoryPawn::StaticClass(), TOptional<int32>()));

	// Alternate - use blueprint api
	
	// Actor factory descriptor
	AddFactoryClass(UActorFactoryCharacter::StaticClass(),
		NAME_None, INVTEXT("SupermanCharacter"));
	//
	AddFactoryWithAsset(UActorFactoryBasicShape::StaticClass(),
		FAssetData(LoadObject<UStaticMesh>(nullptr, *UActorFactoryBasicShape::BasicSphere.ToString())),
		NAME_None, INVTEXT("ShapeAssetSphere"));
	//
	AddFactoryWithObject(UActorFactoryBasicShape::StaticClass(),
		LoadObject<UStaticMesh>(nullptr, *UActorFactoryBasicShape::BasicCube.ToString()),
		NAME_None, INVTEXT("ShapeObjectCube"));
}
