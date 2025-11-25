// Copyright 2025, Aquanox.

#include "IconReference.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(IconReference)

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

FSimpleIconReference::FSimpleIconReference(const FString& InCode)
	: IconCode(InCode)
{
	const FSlateIcon Icon = MakeSlateIconFromCode(InCode);
	StyleSetName = Icon.GetStyleSetName();
	StyleName = Icon.GetStyleName();
}

FSimpleIconReference::FSimpleIconReference(const FName& InStyleSet, const FName& InStyle)
	: StyleSetName(InStyleSet), StyleName(InStyle)
{
	IconCode = MakeSlateIconCode(InStyleSet, InStyle);
}

FSlateIcon FSimpleIconReference::GetSlateIcon() const
{
	return FSlateIcon(StyleSetName, StyleName);
}

FText FSimpleIconReference::GetDisplayText() const
{
	return FText::FromString(FString::Printf(TEXT("[%s] %s"), *StyleSetName.ToString(), *StyleName.ToString()));
}

const FSlateBrush* FSimpleIconReference::GetBrush() const
{
	return GetSlateIcon().GetIcon();
}

const FString& FSimpleIconReference::GetIconCode() const
{
	return IconCode;
}
