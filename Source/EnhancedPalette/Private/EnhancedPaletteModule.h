// Copyright 2025, Aquanox.

#pragma once

#include "EnhancedPaletteGlobals.h"
#include "Modules/ModuleManager.h"

class FEnhancedPaletteModule : public FDefaultModuleImpl
{
public:
	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("EnhancedPalette");
	}

	static FEnhancedPaletteModule& Get()
	{
		return FModuleManager::Get().GetModuleChecked<FEnhancedPaletteModule>("EnhancedPalette");
	}

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool SupportsDynamicReloading() override { return false; }
private:
	void OnPostEngineInitialized();
	void OnEditorInitialized(double Duration);
};

class FPaletteScopedTimeLogger
{
public:
	enum EMode { START_END, END };

	explicit FPaletteScopedTimeLogger(EMode InMode, FString InMsg, ELogVerbosity::Type InVerbosity = ELogVerbosity::Verbose);
	~FPaletteScopedTimeLogger();
private:
	FString        Msg;
	EMode		   Mode;
	ELogVerbosity::Type Verbosity;
	double         StartTime;
};
