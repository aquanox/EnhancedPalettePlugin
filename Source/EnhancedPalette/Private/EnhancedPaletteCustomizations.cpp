#include "EnhancedPaletteCustomizations.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "EdGraphSchema_K2.h"
#include "EnhancedPaletteLibrary.h"
#include "EnhancedPaletteSettings.h"
#include "K2Node_CallFunction.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "SSearchableCombobox.h"
#include "PrivateAccessHelper.h"
#include "PropertyCustomizationHelpers.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SWrapBox.h"

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
}

void EnhancedPaletteCustomizations::Unregister()
{
	auto& Module = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	Module.UnregisterCustomPropertyTypeLayout(FSimpleIconSelector::StaticStruct()->GetFName());
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
