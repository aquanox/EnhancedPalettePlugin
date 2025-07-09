#include "EnhancedPaletteCustomizations.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "EnhancedPaletteLibrary.h"
#include "EnhancedPaletteSettings.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "SSearchableCombobox.h"
#include "PrivateAccessHelper.h"
#include "PropertyCustomizationHelpers.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

using FBrushResourcesMap = TMap<FName, FSlateBrush*>;
UE_DEFINE_PRIVATE_MEMBER_PTR(FBrushResourcesMap, GBrushResourcesMap, FSlateStyleSet, BrushResources);

static FString MakeSlateIconCode(const FName& InStyleSet, const FName& InStyle)
{
	return FString::Printf(TEXT("%s;%s"), *InStyleSet.ToString(), *InStyle.ToString());
}

static FSlateIcon MakeSlateIconFromCode(const FString& InCode)
{
	if (!InCode.IsEmpty())
	{
		FString StyleSet, Name;
		if (InCode.Split(TEXT(";"), &StyleSet, &Name))
		{
			return FSlateIcon(*StyleSet, *Name);
		}
	}
	return FSlateIcon();
}

FSimpleIconSelector::FSimpleIconSelector(const FString& InCode)
	: IconCode(InCode)
{
	FSlateIcon Tmp = MakeSlateIconFromCode(InCode);
	StyleSetName = Tmp.GetStyleSetName();
	StyleName = Tmp.GetStyleName();
}

FSimpleIconSelector::FSimpleIconSelector(const FName& InStyleSet, const FName& InStyle)
	: StyleSetName(InStyleSet), StyleName(InStyle)
{
	IconCode = MakeSlateIconCode(InStyleSet, InStyle);
}


FSlateIcon FSimpleIconSelector::ToSlateIcon() const
{
#if WITH_GATHER_ITEMS_MAGIC
	return FSlateIcon(StyleSetName, StyleName);
#else
	return MakeSlateIconFromCode(IconCode);
#endif
}

FText FSimpleIconSelector::ToDisplayText() const
{
#if WITH_GATHER_ITEMS_MAGIC
	return FText::FromString(FString::Printf(TEXT("[%s] %s"), *StyleSetName.ToString(), *StyleName.ToString()));
#else
	return FText::FromString(FString(IconCode).Replace(TEXT(";"), TEXT(" ")));
#endif
}

// =================================================================================================
// =================================================================================================

void EnhancedPaletteCustomizations::Register()
{
	auto& Module = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	Module.RegisterCustomPropertyTypeLayout(FSimpleIconSelector::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FSimpleIconSelectorCustomization::Make));

	FRegisterCustomClassLayoutParams Params {  TNumericLimits<int32>::Max() };
	Module.RegisterCustomClassLayout(UEnhancedPaletteSettings::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FEnhancedPaletteSettingsCustomization::Make), Params);
}

void EnhancedPaletteCustomizations::Unregister()
{
	auto& Module = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	Module.UnregisterCustomPropertyTypeLayout(FSimpleIconSelector::StaticStruct()->GetFName());
	Module.UnregisterCustomPropertyTypeLayout(UEnhancedPaletteSettings::StaticClass()->GetFName());
}

// =================================================================================================
// =================================================================================================

TSharedRef<IDetailCustomization> FEnhancedPaletteSettingsCustomization::Make()
{
	return MakeShared<ThisClass>();
}

void FEnhancedPaletteSettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.GetObjectsBeingCustomized(/*out*/ SelectedObjectsList);
	// TBD:  Why they were removing archetypes?
	SelectedObjectsList.RemoveAllSwap([](TWeakObjectPtr<UObject> ObjPtr) { return !ObjPtr.IsValid(); });

	TArray<UFunction*> CallInEditorFunctions;
	FEnhancedPaletteSettingsCustomization::GetCallInEditorFunctionsForClass(
		DetailBuilder.GetBaseClass(),
		CallInEditorFunctions);

	PropertyCustomizationHelpers::AddFunctionCallWidgets(
		DetailBuilder,
		CallInEditorFunctions,
		FPropertyFunctionCallDelegates(
			FPropertyFunctionCallDelegates::FOnGetExecutionContext::CreateSP(this, &ThisClass::GetFunctionCallExecutionContext)
		));
}

void FEnhancedPaletteSettingsCustomization::GetCallInEditorFunctionsForClass(const UClass* InClass, TArray<UFunction*>& OutCallInEditorFunctions)
{
	// metadata tag for defining sort order of function buttons within a Category
	static const FName NAME_DisplayPriority("DisplayPriority");

	// Get all of the functions we need to display (done ahead of time so we can sort them)
	for (TFieldIterator<UFunction> FunctionIter(InClass, EFieldIterationFlags::IncludeSuper); FunctionIter; ++FunctionIter)
	{
		const UFunction* TestFunction = *FunctionIter;

		const bool bCanCall = TestFunction && TestFunction->HasMetaData(TEXT("FixedCallInEditor")) && (TestFunction->ParmsSize == 0);
		if (bCanCall)
		{
			const FName FunctionName = TestFunction->GetFName();
			const bool bFunctionAlreadyAdded = OutCallInEditorFunctions.ContainsByPredicate([&FunctionName](UFunction*& Func)
			{
				return Func->GetFName() == FunctionName;
			});
			
			if (!bFunctionAlreadyAdded)
			{
				OutCallInEditorFunctions.Add(*FunctionIter);
			}
		}
	}

	if (OutCallInEditorFunctions.IsEmpty())
	{
		return;
	}

	// FBlueprintMetadata::MD_FunctionCategory
	static const FName NAME_FunctionCategory(TEXT("Category"));

	// Sort the functions by category and then by DisplayPriority meta tag, and then by name
	OutCallInEditorFunctions.Sort([](const UFunction& A, const UFunction& B)
	{
		const int32 CategorySort = A.GetMetaData(NAME_FunctionCategory).Compare(B.GetMetaData(NAME_FunctionCategory));
		if (CategorySort != 0)
		{
			return (CategorySort <= 0);
		}
		else
		{
			const FString DisplayPriorityAStr = A.GetMetaData(NAME_DisplayPriority);
			int32 DisplayPriorityA = (DisplayPriorityAStr.IsEmpty() ? MAX_int32 : FCString::Atoi(*DisplayPriorityAStr));
			if (DisplayPriorityA == 0 && !FCString::IsNumeric(*DisplayPriorityAStr))
			{
				DisplayPriorityA = MAX_int32;
			}

			const FString DisplayPriorityBStr = B.GetMetaData(NAME_DisplayPriority);
			int32 DisplayPriorityB = (DisplayPriorityBStr.IsEmpty() ? MAX_int32 : FCString::Atoi(*DisplayPriorityBStr));
			if (DisplayPriorityB == 0 && !FCString::IsNumeric(*DisplayPriorityBStr))
			{
				DisplayPriorityB = MAX_int32;
			}

			return (DisplayPriorityA == DisplayPriorityB) ? (A.GetName() <= B.GetName()) : (DisplayPriorityA <= DisplayPriorityB);
		}
	});
}

// =================================================================================================
// =================================================================================================

TSharedRef<IPropertyTypeCustomization> FSimpleIconSelectorCustomization::Make()
{
	return MakeShared<ThisClass>();
}

inline bool IsValidBrush(const FName& InKey, const FSlateBrush* InBrush)
{
	const FVector2D Icon20x20(20, 20);
	
	FString KeyString = InKey.ToString();
	if (KeyString.EndsWith(TEXT(".Small"), ESearchCase::IgnoreCase)
		|| KeyString.Contains(TEXT("Overlay"), ESearchCase::IgnoreCase)
		|| KeyString.Contains(TEXT("Status"), ESearchCase::IgnoreCase))
		return false;

	// TBD: hardcode size as any bigger image breaks ui in 5.5+
	if (InBrush && InBrush->GetImageSize() == Icon20x20 && InBrush->GetDrawType() == ESlateBrushDrawType::Image)
		return true;

	return false;
}

#if WITH_GATHER_ITEMS_MAGIC

TArray<TSharedPtr<FSimpleIconSelector>> FSimpleIconSelectorCustomization::GatherIcons()
{
	TArray<TSharedPtr<FSimpleIconSelector>> Result;
	if (const ISlateStyle* Style = FSlateStyleRegistry::FindSlateStyle(FAppStyle::GetAppStyleSetName()))
	{
		const FBrushResourcesMap& Brushes = static_cast<const FSlateStyleSet*>(Style)->*GBrushResourcesMap;
		for (const TPair<FName, FSlateBrush*>& Pair : Brushes)
		{
			if (IsValidBrush(Pair.Key, Pair.Value))
			{
				auto Item = MakeShared<FSimpleIconSelector>();

				Item->IconCode = MakeSlateIconCode(Style->GetStyleSetName(), Pair.Key);
				Item->StyleSetName = Style->GetStyleSetName();
				Item->StyleName = Pair.Key;
				
				Result.Emplace(MoveTemp(Item));
			}
		}
	}
	Algo::Sort(Result, [](const TSharedPtr<FSimpleIconSelector>& Left, const TSharedPtr<FSimpleIconSelector>& Right)
	{
		return Left->IconCode < Right->IconCode;
	});
	return Result;
}

#else

TArray<FString> FSimpleIconSelectorCustomization::GatherIcons()
{
	TArray<FString> Result;
	if (const ISlateStyle* Style = FSlateStyleRegistry::FindSlateStyle(FAppStyle::GetAppStyleSetName()))
	{
		const FBrushResourcesMap& Brushes = static_cast<const FSlateStyleSet*>(Style)->*GBrushResourcesMap;
		for (const TPair<FName, FSlateBrush*>& Pair : Brushes)
		{
			if (IsValidBrush(Pair.Key, Pair.Value))
			{
				Result.Add(MakeSlateIconCode(Style->GetStyleSetName(), Pair.Key));
			}
		}
	}
	Algo::Sort(Result);
	return Result;
}

#endif

void FSimpleIconSelectorCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	Handle = PropertyHandle;
	Font = FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont"));

#if WITH_GATHER_ITEMS_MAGIC
	ComboItemList = FSimpleIconSelectorCustomization::GatherIcons();
	const TArray<TSharedPtr<FString>>* TheSource = reinterpret_cast<const TArray<TSharedPtr<FString>>*>(&ComboItemList);
#else
	for (const FString& KnownItem : FSimpleIconSelectorCustomization::GatherIcons())
	{
		ComboItemList.Emplace(MakeShared<FString>(KnownItem));
	}
	const TArray<TSharedPtr<FString>>* TheSource = &ComboItemList;
#endif
	
	PropertyHandle->SetOnPropertyResetToDefault(FSimpleDelegate::CreateSP(this, &ThisClass::OnSelectionChangedInternal, TSharedPtr<FString>(), ESelectInfo::Direct));

	HeaderRow
		.IsEnabled(MakeAttributeSP(this, &ThisClass::CanEdit))
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(TOptional<float>())
		.MaxDesiredWidth(TOptional<float>())
		.HAlign(HAlign_Fill)
		[
			SAssignNew(Combobox, SSearchableComboBox)
			.MaxListHeight(450.f)
			.Content()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2, 2, 4, 2)
				[
					SNew(SImage).Image(MakeAttributeSP(this, &ThisClass::GetDisplayIcon)).DesiredSizeOverride(FVector2D(20, 20))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(Font).Text(MakeAttributeSP(this, &ThisClass::GetDisplayText))
				]
			]
			.OptionsSource(TheSource)
			.OnGenerateWidget(this, &ThisClass::OnGenerateComboWidget)
			.OnSelectionChanged(this, &ThisClass::OnSelectionChangedInternal)
			.SearchVisibility(EVisibility::Visible)
		];
}

void FSimpleIconSelectorCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

bool FSimpleIconSelectorCustomization::CanEdit() const
{
	return Handle.IsValid() && !Handle->IsEditConst();
}

const FSlateBrush* FSimpleIconSelectorCustomization::GetDisplayIcon() const
{
	void* DataPtr = nullptr;
	if (Handle->GetValueData(DataPtr) == FPropertyAccess::Success)
	{
		const FSimpleIconSelector* Ptr = static_cast<FSimpleIconSelector*>(DataPtr);
		return Ptr->ToSlateIcon().GetIcon();
	}
	return FSlateIcon().GetIcon();
}

FText FSimpleIconSelectorCustomization::GetDisplayText() const
{
	void* DataPtr = nullptr;
	if (Handle->GetValueData(DataPtr) == FPropertyAccess::Success)
	{
		const FSimpleIconSelector* Ptr = static_cast<FSimpleIconSelector*>(DataPtr);
		return Ptr->ToDisplayText();
	}
	return FText::GetEmpty();
}

TSharedRef<SWidget> FSimpleIconSelectorCustomization::OnGenerateComboWidget(TSharedPtr<FString> InElement)
{
#if WITH_GATHER_ITEMS_MAGIC
	const FSimpleIconSelector* TheItem = reinterpret_cast<const FSimpleIconSelector*>(InElement.Get());
	FSlateIcon TheIcon( TheItem->StyleSetName, TheItem->StyleName );
	FText TheText = TheItem->ToDisplayText();
#else
	FSlateIcon TheIcon = MakeSlateIconFromCode(*InElement);
	FText TheText = FSimpleIconSelector(*InElement).ToDisplayText();
#endif
	
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2, 2, 4, 2)
		[
			SNew(SImage).Image(TheIcon.GetIcon()).DesiredSizeOverride(FVector2D(20, 20))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock).Font(Font).Text(TheText)
		];
}

void FSimpleIconSelectorCustomization::OnSelectionChangedInternal(TSharedPtr<FString> Selected, ESelectInfo::Type)
{
	if (!Selected.IsValid())
	{
		Handle->SetValueFromFormattedString(TEXT(""));
		return;
	}
	
	const FSimpleIconSelector* TargetStruct = nullptr;
#if WITH_GATHER_ITEMS_MAGIC
	TargetStruct = reinterpret_cast<const FSimpleIconSelector*>(Selected.Get());
#else
	FSimpleIconSelector Tmp( *Selected );
	TargetStruct = &Tmp;
#endif

	FString FormattedString;
	StaticStruct<FSimpleIconSelector>()->ExportText(FormattedString, TargetStruct, nullptr, nullptr, PPF_None, nullptr, false);
	Handle->SetValueFromFormattedString(FormattedString);
}
