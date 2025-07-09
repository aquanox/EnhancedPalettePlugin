// Copyright 2025, Aquanox.

#include "ExampleNativeCategory.h"
#include "Editor.h"
#include "ActorFactories/ActorFactoryBasicShape.h"
#include "ActorFactories/ActorFactoryCharacter.h"
#include "ActorFactories/ActorFactoryPawn.h"

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

void UExampleNativeCategory::NativeGatherItems(TArray<TConfigPlaceableItem>& OutResult)
{
	// Preferred - construct FPlaceableItem directly with desired params
	// More Examples can be found in PlacementModeModule.cpp FPlacementModeModule::StartupModule
	
	// PlaceableItem arg descriptor
	AddDescriptor_Item<FPlaceableItem>(*UActorFactoryEmptyActor::StaticClass(), TOptional<int32>());
	
	// PlaceableItem descriptor
	AddDescriptor_Native(MakeShared<FPlaceableItem>(*UActorFactoryPawn::StaticClass(), TOptional<int32>()));

	// Alternate - use blueprint api
	
	// Actor factory descriptor
	AddDescriptor_FactoryClass(UActorFactoryCharacter::StaticClass(),
		INVTEXT("Superman"));
	//
	AddDescriptor_FactoryAsset(UActorFactoryBasicShape::StaticClass(),
		FAssetData(LoadObject<UStaticMesh>(nullptr, *UActorFactoryBasicShape::BasicSphere.ToString())),
		INVTEXT("ShapeAsset"));
	//
	AddDescriptor_FactoryObject(UActorFactoryBasicShape::StaticClass(),
		LoadObject<UStaticMesh>(nullptr, *UActorFactoryBasicShape::BasicCube.ToString()),
		INVTEXT("ShapeObject"));
}
