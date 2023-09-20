// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomStyle/SuperManagerStyle.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"

FName FSuperManagerStyle::StyleSetName = FName("SuperManagerStyle");
TSharedPtr<FSlateStyleSet> FSuperManagerStyle::CreatedSlateStyleSet = nullptr;

void FSuperManagerStyle::InitializeIcons()
{
	if (!CreatedSlateStyleSet.IsValid())
	{
		CreatedSlateStyleSet = CreateSlateStyleSet();
		FSlateStyleRegistry::RegisterSlateStyle(*CreatedSlateStyleSet);
	}
}

TSharedRef<FSlateStyleSet> FSuperManagerStyle::CreateSlateStyleSet()
{
	TSharedRef<FSlateStyleSet> CustomStyleSet = MakeShareable(new FSlateStyleSet(StyleSetName));

	const FString IconDirectory =
		IPluginManager::Get().FindPlugin(TEXT("SuperManager"))->GetBaseDir()/"Resources";

	CustomStyleSet->SetContentRoot(IconDirectory);

	const FVector2D Icon16x16(32.f, 32.f);
	CustomStyleSet->Set(
		"ConteneBrowser.DeleteUnusedAssets",
		new FSlateImageBrush(IconDirectory / "banana2.png", Icon16x16) );


	CustomStyleSet->Set(
		"ConteneBrowser.DeleteEmptyFolder",
		new FSlateImageBrush(IconDirectory / "apple2.png", Icon16x16));


	CustomStyleSet->Set(
		"ConteneBrowser.AdvancedDeletion",
		new FSlateImageBrush(IconDirectory / "happycat2.png", Icon16x16));

	return CustomStyleSet;
}

void FSuperManagerStyle::ShutDown()
{
	if (CreatedSlateStyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*CreatedSlateStyleSet);
		CreatedSlateStyleSet.Reset();
	}
}
