// Copyright 2025, Aquanox.

#include "IconReferenceCustomization.h"

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

// =================================================================================================
// =================================================================================================

void EnhancedPaletteCustomizations::Register()
{
	auto& Module = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	Module.RegisterCustomPropertyTypeLayout(FSimpleIconReference::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FSimpleIconReferenceCustomization::Make));
}

void EnhancedPaletteCustomizations::Unregister()
{
	auto& Module = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	Module.UnregisterCustomPropertyTypeLayout(FSimpleIconReference::StaticStruct()->GetFName());
}

// =================================================================================================
// =================================================================================================

TSharedRef<IPropertyTypeCustomization> FSimpleIconReferenceCustomization::Make()
{
	return MakeShared<ThisClass>();
}

static bool IsValidBrush(const FName& InKey, const FSlateBrush* InBrush)
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

void FSimpleIconReferenceCustomization::GatherIcons(TFunction<void(FName, FName)> Func)
{
	if (const ISlateStyle* Style = FSlateStyleRegistry::FindSlateStyle(FAppStyle::GetAppStyleSetName()))
	{
		const FBrushResourcesMap& Brushes = static_cast<const FSlateStyleSet*>(Style)->*GBrushResourcesMap;
		for (const TPair<FName, FSlateBrush*>& Pair : Brushes)
		{
			if (IsValidBrush(Pair.Key, Pair.Value))
			{
				Func(Style->GetStyleSetName(), Pair.Key);
			}
		}
	}
}

#if WITH_MAGIC_COMBO_CAST

void FSimpleIconReferenceCustomization::BuildComboItemList(TArray<TSharedPtr<FString>>& Output) const
{
	GatherIcons([&](FName StyleSet, FName IconName)
	{
		FSimpleIconReference* Item = new FSimpleIconReference(StyleSet, IconName);
		Output.Add(MakeShareable<FString>(reinterpret_cast<FString*>(Item)));
	});
	Algo::Sort(Output, [](const TSharedPtr<FString>& Left, const TSharedPtr<FString>& Right)
	{
		FSimpleIconReference* ItemLeft = reinterpret_cast<FSimpleIconReference*>(Left.Get());
		FSimpleIconReference* ItemRight = reinterpret_cast<FSimpleIconReference*>(Right.Get());
		return ItemLeft->GetIconCode() < ItemRight->GetIconCode();
	});
}

void FSimpleIconReferenceCustomization::ExtractIcon(const TSharedPtr<FString>& Element, FSimpleIconReference& Output) const
{
	Output = *reinterpret_cast<FSimpleIconReference*>(Element.Get());
}

#else

void FSimpleIconReferenceCustomization::BuildComboItemList(TArray<TSharedPtr<FString>>& Output) const
{
	GatherIcons([&](FName StyleSet, FName IconName)
	{
		Output.Add(MakeShared<FString>(FSimpleIconReference(StyleSet, IconName).GetIconCode()));
	});
	Algo::Sort(Output, [](const TSharedPtr<FString>& Left, const TSharedPtr<FString>& Right)
	{
		return *Left < *Right;
	});
}

void FSimpleIconReferenceCustomization::ExtractIcon(const TSharedPtr<FString>& Element, FSimpleIconReference& Output) const
{
	Output = FSimpleIconReference(*Element);
}

#endif

void FSimpleIconReferenceCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	Handle = PropertyHandle;

	BuildComboItemList(ComboItemList);

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
		SNew(SBox)
		.Padding(0, 3, 0, 3)
		.VAlign(VAlign_Center)
		[
			SAssignNew(Combobox, SSearchableComboBox)
			.MaxListHeight(450.f)
			.Content()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2, 2, 6, 2)
				[
					SNew(SImage).Image(MakeAttributeSP(this, &ThisClass::GetDisplayIcon)).DesiredSizeOverride(FVector2D(20, 20))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont"))).Text(MakeAttributeSP(this, &ThisClass::GetDisplayText))
				]
			]
			.OptionsSource(&ComboItemList)
			.OnGenerateWidget(this, &ThisClass::OnGenerateComboWidget)
			.OnSelectionChanged(this, &ThisClass::OnSelectionChangedInternal)
			.SearchVisibility(EVisibility::Visible)
		]
	];
}

void FSimpleIconReferenceCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

bool FSimpleIconReferenceCustomization::CanEdit() const
{
	return Handle.IsValid() && !Handle->IsEditConst();
}

const FSlateBrush* FSimpleIconReferenceCustomization::GetDisplayIcon() const
{
	void* DataPtr = nullptr;
	if (Handle->GetValueData(DataPtr) == FPropertyAccess::Success)
	{
		return static_cast<const FSimpleIconReference*>(DataPtr)->GetBrush();
	}
	return FSlateIcon().GetIcon();
}

FText FSimpleIconReferenceCustomization::GetDisplayText() const
{
	void* DataPtr = nullptr;
	if (Handle->GetValueData(DataPtr) == FPropertyAccess::Success)
	{
		return static_cast<const FSimpleIconReference*>(DataPtr)->GetDisplayText();
	}
	return FText::GetEmpty();
}

TSharedRef<SWidget> FSimpleIconReferenceCustomization::OnGenerateComboWidget(TSharedPtr<FString> InElement)
{
	FSimpleIconReference TheItem;
	ExtractIcon(InElement, TheItem);

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2, 2, 6, 2)
		[
			SNew(SImage).Image(TheItem.GetBrush()).DesiredSizeOverride(FVector2D(20, 20))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock).Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont"))).Text(TheItem.GetDisplayText())
		];
}

void FSimpleIconReferenceCustomization::OnSelectionChangedInternal(TSharedPtr<FString> Selected, ESelectInfo::Type)
{
	if (!Selected.IsValid())
	{
		Handle->SetValueFromFormattedString(TEXT(""));
		return;
	}

	FSimpleIconReference TheItem;
	ExtractIcon(Selected, TheItem);

	FString FormattedString;
	StaticStruct<FSimpleIconReference>()->ExportText(FormattedString, &TheItem, nullptr, nullptr, PPF_None, nullptr, false);
	Handle->SetValueFromFormattedString(FormattedString);
}
