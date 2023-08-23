// Copyright Epic Games, Inc. All Rights Reserved.

#include "SuperManager.h"
#include "ContentBrowserModule.h"
#include "DebugHeader.h"
#include "EditorAssetLibrary.h"
#include "ObjectTools.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"

#define LOCTEXT_NAMESPACE "FSuperManagerModule"

void FSuperManagerModule::StartupModule()
{
	InitCBMenuExtention();

}

#pragma region	ContentBrowserMenuWxtention

void FSuperManagerModule::InitCBMenuExtention()
{
	FContentBrowserModule& ContentBrowserModule =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	TArray<FContentBrowserMenuExtender_SelectedPaths>& ContentBrowserModuleMenuExtenders =
		ContentBrowserModule.GetAllPathViewContextMenuExtenders();

	/*FContentBrowserMenuExtender_SelectedPaths CustomCBMenuDelegate;
	CustomCBMenuDelegate.BindRaw(this, &FSuperManagerModule::CustomCBMenuExtender);

	ContentBrowserModuleMenuExtenders.Add(CustomCBMenuDelegate);*/

	ContentBrowserModuleMenuExtenders.Add(
		FContentBrowserMenuExtender_SelectedPaths::CreateRaw(this, &FSuperManagerModule::CustomCBMenuExtender));
}

TSharedRef<FExtender> FSuperManagerModule::CustomCBMenuExtender(const TArray<FString>& SelectedPaths)
{
	TSharedRef<FExtender> MenuExtender (new FExtender());

	if (SelectedPaths.Num() > 0)
	{
		MenuExtender->AddMenuExtension(
			FName("Delete"),				// Extention hook, position to insert
			EExtensionHook::After,			// inserting before or after
			TSharedPtr<FUICommandList>(),	// custom hot key
			// Second binding, wwill define details for this menu entry
			FMenuExtensionDelegate::CreateRaw(this, &FSuperManagerModule::AddCBMenuEntry));

		FolderPathsSelected = SelectedPaths;
	}

	return MenuExtender;
}


// Define details for the custom menu entry
void FSuperManagerModule::AddCBMenuEntry(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Delete Unused Assets")),						// Title text for menu entry
		FText::FromString(TEXT("Safty delete all unused assets under folder")),	// Tool tip text
		FSlateIcon(),	// Custom icon
		// The actual funcion execute
		FExecuteAction::CreateRaw(this, &FSuperManagerModule::OnDeleteUnusedAssetButtonClicked)
	);

	MenuBuilder.AddMenuEntry
	(
		FText::FromString(TEXT("Delete Empty Folder")),						// Title text for menu entry
		FText::FromString(TEXT("Safty delete all empty folders")),	// Tool tip text
		FSlateIcon(),	// Custom icon
		// The actual funcion execute
		FExecuteAction::CreateRaw(this, &FSuperManagerModule::OnDeleteEmptyFoldersButtonClicked)
	);
}

void FSuperManagerModule::OnDeleteUnusedAssetButtonClicked()
{
	if (FolderPathsSelected.Num() > 1)
	{
		DebugHeader::ShowMsgDialog(EAppMsgType::Ok, TEXT("You can only do this to one folder!"));
		return;
	}

	TArray<FString> AssetsPathNames = UEditorAssetLibrary::ListAssets(FolderPathsSelected[0]);

	if (AssetsPathNames.Num() == 0)
	{
		// NO ASSETS!
		DebugHeader::ShowMsgDialog(EAppMsgType::Ok, TEXT("No Asset found under selected folder"));
		return;
	}

	EAppReturnType::Type ConfirmResult =
		DebugHeader::ShowMsgDialog(	EAppMsgType::YesNo, TEXT("A Total of ")
									+ FString::FromInt(AssetsPathNames.Num())
									+ TEXT(" assets need to be checked. \n Would you like to proceed? "), false);

	if (ConfirmResult == EAppReturnType::No) return;

	FixUpRedirectors();

	TArray<FAssetData> UnusedAssetsDataArray;
	
	for (const auto& AssetPathName : AssetsPathNames)
	{
		// Do not touch root Folder!
		// __ExternalActors__ �� __ExternalObject__ �� UE5 ���� �߰��� ����.
		// Developers, Collections �� ���������� �ǵ� x
		if (AssetPathName.Contains(TEXT("Developers")) ||
			AssetPathName.Contains(TEXT("Collections")) ||
			AssetPathName.Contains(TEXT("__ExternalActors__")) ||
			AssetPathName.Contains(TEXT("__ExternalObject__")) )
		{
			continue;
		}

		if (!UEditorAssetLibrary::DoesAssetExist(AssetPathName)) continue;
		
		TArray<FString> AssetReferencers = 
			UEditorAssetLibrary::FindPackageReferencersForAsset(AssetPathName);

		if (AssetReferencers.Num() == 0)
		{
			const FAssetData UnusedAssetData = UEditorAssetLibrary::FindAssetData(AssetPathName);
			UnusedAssetsDataArray.Add(UnusedAssetData);
		}
	}

	if (UnusedAssetsDataArray.Num() > 0)
	{
		ObjectTools::DeleteAssets(UnusedAssetsDataArray);
	}
	else
	{
		DebugHeader::ShowMsgDialog(EAppMsgType::Ok, TEXT("No Unused asset found under selected folder"), false);
	}


	EAppReturnType::Type ConfirmContinueEraseFolderResult =
		DebugHeader::ShowMsgDialog(EAppMsgType::YesNo, TEXT("Would you like delete Empty Folder?"), false);
	
	if (ConfirmContinueEraseFolderResult == EAppReturnType::No) return;

	OnDeleteEmptyFoldersButtonClicked();
}

void FSuperManagerModule::OnDeleteEmptyFoldersButtonClicked()
{
	FixUpRedirectors();

	TArray<FString> FolderPathsArray = UEditorAssetLibrary::ListAssets(FolderPathsSelected[0], true, true);
	uint32 Counter = 0;

	FString EmptyFolderPathsNames;
	TArray<FString> EmptyFoldersPathsArray;

	for (const auto& FolderPath : FolderPathsArray)
	{
		if (FolderPath.Contains(TEXT("Developers")) ||
			FolderPath.Contains(TEXT("Collections")) ||
			FolderPath.Contains(TEXT("__ExternalActors__")) ||
			FolderPath.Contains(TEXT("__ExternalObject__")))
		{
			continue;
		}

		if (!UEditorAssetLibrary::DoesDirectoryExist(FolderPath)) continue;

		if (!UEditorAssetLibrary::DoesDirectoryHaveAssets(FolderPath))
		{
			// for �� ���̹Ƿ� ���� ��θ� �ֱ� ���ؼ� ������ ���� �Ѵ�.
			EmptyFolderPathsNames.Append(FolderPath);
			EmptyFolderPathsNames.Append(TEXT("\n"));

			EmptyFoldersPathsArray.Add(FolderPath);
		}
	}

	if (EmptyFoldersPathsArray.Num() == 0)
	{
		DebugHeader::ShowMsgDialog(EAppMsgType::Ok,
			TEXT("No empty folder found under selected folder"), false);
		return;
	}

	EAppReturnType::Type ConfirmResult = 
		DebugHeader::ShowMsgDialog(EAppMsgType::OkCancel,
			TEXT("Empty Folders found in : \n") + EmptyFolderPathsNames + TEXT("\n Would you like delete all?"),
			false);

	if (ConfirmResult == EAppReturnType::Cancel) return;

	for(const auto& EmptyFolderPath : EmptyFoldersPathsArray)
	{
		UEditorAssetLibrary::DeleteDirectory(EmptyFolderPath) ?
			++Counter : DebugHeader::Print(TEXT("Failed to delete : ") + EmptyFolderPath, FColor::Red);
	}

	if (Counter > 0)
	{
		DebugHeader::ShowNotifyInfo( TEXT("Successfullt deleted : ") + FString::FromInt(Counter) + TEXT(" Folders."));
	}
}

void FSuperManagerModule::FixUpRedirectors()
{
	TArray<UObjectRedirector*> RedirectorsToFixArray;

	// ������ ������ ���� ��θ� ���������� FAssetRegistryModule �� �����ؾ���.
	// �켱 ����� ���´�.
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	// �ϴ��� GetAssets �� ����Ҷ� ���͸� ����Ѵ�.
	// ���� ���ֺ��� ���͸� �Ǵ� ������ ���� �� ����.
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace("/Game");
	Filter.ClassNames.Emplace("ObjectRedirector");

	// ���Ϳ� �ش�Ǵ� ������ TArray ���·� �����Ѵ�.
	TArray<FAssetData> OutRedirectors;
	AssetRegistryModule.Get().GetAssets(Filter, OutRedirectors);


	// UObjectRedirector �� ��ü�̸��� �ٸ� ��Ű���� �׷����� �ٲ��
	// �ش� ��ü�� ���� �ܺ� ������ ã�� �� ����.
	// ��ü�� �ٲ� ������ ã���� RedirectorsToFixArray �� �־���.
	for (const auto& RedirectorData : OutRedirectors)
	{
		if (UObjectRedirector* RedirectorToFix = Cast<UObjectRedirector>(RedirectorData.GetAsset()))
		{
			RedirectorsToFixArray.Add(RedirectorToFix);
		}
	}

	// AssetTools ����� ȣ����.
	FAssetToolsModule& AssetToolsModule =
		FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));


	// ȣ���� ����� ������ ��θ� �ٲ���.
	AssetToolsModule.Get().FixupReferencers(RedirectorsToFixArray);
}

#pragma endregion


void FSuperManagerModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSuperManagerModule, SuperManager)