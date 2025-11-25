// Copyright 2025, Aquanox.

#include "ExampleNativeCategory.h"
#include "Editor.h"
#include "ActorFactories/ActorFactoryBasicShape.h"
#include "ActorFactories/ActorFactoryBlueprint.h"
#include "ActorFactories/ActorFactoryCharacter.h"
#include "ActorFactories/ActorFactoryPawn.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "GameFramework/Pawn.h"
#include "Subsystems/PlacementSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExampleNativeCategory)

namespace FeatureSwitches
{
	constexpr bool bRegisterPeriodicUpdate = false;
	constexpr bool bRegisterDelegates = false;
}

UExampleNativeCategory::UExampleNativeCategory()
{
    ShortDisplayName = INVTEXT("Native");
	DisplayName = INVTEXT("This is an example of category");
	DisplayIcon = FSimpleIconReference("EditorStyle", "ContentPalette.ShowParticles");

	// can register for periodic updates
	if (FeatureSwitches::bRegisterPeriodicUpdate)
	{
		bTickable = true;
		TickInterval = 10.f;
	}
}

void UExampleNativeCategory::NativeInitialize()
{
	// Can register to delegates that will trigger content updates 
	if (FeatureSwitches::bRegisterDelegates)
	{
		// Use only AddUObject or AddWeakLambda as category object may be reinstantiated.
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
	ExampleGatherExplicit();
		
	ExampleGatherBlueprintsOfClass();

	ExampleGatherAssetsOfClass();
}

void UExampleNativeCategory::ExampleGatherExplicit()
{
	// Preferred - construct FPlaceableItem directly with desired params
	// More Examples can be found in PlacementModeModule.cpp FPlacementModeModule::StartupModule
	
	// PlaceableItem arg descriptor
	AddPlaceableItem<FPlaceableItem>(*UActorFactoryEmptyActor::StaticClass(), TOptional<int32>());
	
	// PlaceableItem descriptor
	AddPlaceableItemPtr(MakeShared<FPlaceableItem>(*UActorFactoryPawn::StaticClass(), TOptional<int32>()));

	// Alternate - use shortcuts
	
	// Actor factory descriptor
	AddFactoryClass(UActorFactoryCharacter::StaticClass(),
		NAME_None, INVTEXT("SupermanCharacter"));
	
	// Actor factory and Asset Data
	AddFactoryWithAsset(UActorFactoryBasicShape::StaticClass(),
		FAssetData(LoadObject<UStaticMesh>(nullptr, *UActorFactoryBasicShape::BasicSphere.ToString())),
		NAME_None, INVTEXT("ShapeAssetSphere"));
	
	// Actor factory and Asset Object
	AddFactoryWithObject(UActorFactoryBasicShape::StaticClass(),
		LoadObject<UStaticMesh>(nullptr, *UActorFactoryBasicShape::BasicCube.ToString()),
		NAME_None, INVTEXT("ShapeObjectCube"));
}

/**
 * Searching for blueprints with base type of APawn or descendants
 */
void UExampleNativeCategory::ExampleGatherBlueprintsOfClass()
{
	// Lambda that will register found asset for category
	UPlacementSubsystem* const PlacementSubsystem = GEditor->GetEditorSubsystem<UPlacementSubsystem>();
	auto AddAsset = [this, PlacementSubsystem](const FAssetData& AssetData)
	{
		// Query PlacementSubsystem to find appropriate ActorFactory for the asset
		auto Factory = PlacementSubsystem->FindAssetFactoryFromAssetData(AssetData);
		// Or directly use UActorFactoryBlueprint
		//auto Factory = PlacementSubsystem->GetAssetFactoryFromFactoryClass(UActorFactoryBlueprint::StaticClass());
		if (Factory && Factory->CanPlaceElementsFromAssetData(AssetData))
		{
			auto Item = MakeShared<FPlaceableItem>(Factory, AssetData);
			// Item can be further customized here
			Item->NativeName = FString("GB_") + Item->NativeName;
			Item->DisplayName = FText::FromString(FString("GB_") + Item->DisplayName.ToString());
			Item->SortOrder = 10;
			// And finally added for the gather cycle
			AddPlaceableItemPtr(Item);
		}
	};
	
	UClass* const ExpectedParentBaseClass = APawn::StaticClass();

	// Build the asset search filter 
	FARFilter Filter;
	Filter.ClassPaths.Add(FTopLevelAssetPath(FName("/Script/Engine"), FName("Blueprint")));
	Filter.PackagePaths.Add(TEXT("/Game"));
	Filter.TagsAndValues.Add(TEXT("NativeParentClass"));
	Filter.bIncludeOnlyOnDiskAssets = true;

	// Enumerate all assets to find any blueprints who inherit from native classes directly - or from other blueprints.
	IAssetRegistry::GetChecked().EnumerateAssets(Filter, [&](const FAssetData& Asset)
	{
		FAssetDataTagMapSharedView::FFindTagResult NativeParentClass = Asset.TagsAndValues.FindTag(TEXT("NativeParentClass"));
		if (NativeParentClass.IsSet())
		{
			FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(NativeParentClass.GetValue());
			if (UClass* ParentClass = FindObjectSafe<UClass>(nullptr, *ClassObjectPath, true))
			{
				if (ParentClass->IsChildOf(ExpectedParentBaseClass))
				{
					AddAsset(Asset);
					return true;
				}
			}
		}
		return true;
	});
}

void UExampleNativeCategory::ExampleGatherAssetsOfClass()
{
	// Since this example searches for meshes to spawn - can already pinpoint desired factory type
	auto SMAFactory = GEditor->GetEditorSubsystem<UPlacementSubsystem>()
							 ->GetAssetFactoryFromFactoryClass(UActorFactoryStaticMesh::StaticClass());
	
	// Build the asset search filter with some parameters
	FARFilter Filter;
	Filter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(TEXT("/Engine/EngineMeshes"));
	Filter.TagsAndValues.Add(TEXT("NaniteEnabled"), TOptional<FString>(TEXT("False")));
	Filter.bIncludeOnlyOnDiskAssets = true;

	// Enumerate all assets matching filter and add them
	IAssetRegistry::GetChecked().EnumerateAssets(Filter, [&](const FAssetData& Asset)
	{
		auto Item = MakeShared<FPlaceableItem>(SMAFactory, Asset, TOptional<int32>(20));
		Item->NativeName = FString("GA_") + Item->NativeName;
		Item->DisplayName = FText::FromString(FString("GA_") + Item->DisplayName.ToString());
		AddPlaceableItemPtr(Item);
		return true;
	});
}

void UExampleNativeCategory::ExampleCustomItemType()
{
	struct FExamplePlaceableItem : public FPlaceableItem
	{
		using FPlaceableItem::FPlaceableItem;

		FExamplePlaceableItem(UClass& InFactory, const FAssetData& InData, TOptional<int32> InSort, TOptional<FText> InDesc)
			: FPlaceableItem(InFactory, InData, NAME_None, NAME_None, TOptional<FLinearColor>(), InSort, InDesc)
		{
			
		}
	};

	UClass* ExampleFactory = UActorFactoryEmptyActor::StaticClass();

	AddPlaceableItemPtr(
		MakeShared<FExamplePlaceableItem>(*ExampleFactory, FAssetData(), 43, INVTEXT("Empty Actor"))
	);
	
	AddPlaceableItem<FExamplePlaceableItem>(
		*ExampleFactory, FAssetData(), 43, INVTEXT("Empty Actor")
	);

	TArray<TSharedPtr<FExamplePlaceableItem>> Locals;
	AddPlaceableItemPtrs(Locals);
}
