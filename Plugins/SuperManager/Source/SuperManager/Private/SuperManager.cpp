// Copyright Epic Games, Inc. All Rights Reserved.

#include "SuperManager.h"
#include "ContentBrowserModule.h"
#include "DebugHeader.h"
#include "EditorAssetLibrary.h"
#include "ObjectTools.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include <Widgets/Docking/SDockTab.h>
#include "SlateWidgets/AdvanceDeletionWidget.h"

#define LOCTEXT_NAMESPACE "FSuperManagerModule"

void FSuperManagerModule::StartupModule()
{
	InitCBMenuExtention();
	RegisterAdvancedDeletionTab();
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

	MenuBuilder.AddMenuEntry
	(
		FText::FromString(TEXT("Advanced Deletion")),						// Title text for menu entry
		FText::FromString(TEXT("List assets by specific condition in a tab for deleting")),	// Tool tip text
		FSlateIcon(),	// Custom icon
		// The actual funcion execute
		FExecuteAction::CreateRaw(this, &FSuperManagerModule::OnAdvancedDeletionButtonClicked)
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
		// __ExternalActors__ 와 __ExternalObject__ 는 UE5 부터 추가된 파일.
		// Developers, Collections 와 마찬가지로 건들 x
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
			// for 문 안이므로 여러 경로를 넣기 위해서 다음과 같이 한다.
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

void FSuperManagerModule::OnAdvancedDeletionButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(FName("AdvancedDeletion"));
}

void FSuperManagerModule::FixUpRedirectors()
{
	TArray<UObjectRedirector*> RedirectorsToFixArray;

	// 콘텐츠 브라우저 내부 경로를 가져오려면 FAssetRegistryModule 에 접근해야함.
	// 우선 모듈을 얻어온다.
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	// 하단의 GetAssets 을 사용할때 필터를 사용한다.
	// 여러 범주별로 필터링 되는 에셋을 만들 수 있음.
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace("/Game");
	Filter.ClassNames.Emplace("ObjectRedirector");

	// 필터에 해당되는 에셋을 TArray 형태로 저장한다.
	TArray<FAssetData> OutRedirectors;
	AssetRegistryModule.Get().GetAssets(Filter, OutRedirectors);


	// UObjectRedirector 는 개체이름이 다른 패키지나 그룹으로 바뀌면
	// 해당 개체에 대한 외부 참조를 찾을 수 있음.
	// 개체가 바뀌어서 참조를 찾으면 RedirectorsToFixArray 에 넣어줌.
	for (const auto& RedirectorData : OutRedirectors)
	{
		if (UObjectRedirector* RedirectorToFix = Cast<UObjectRedirector>(RedirectorData.GetAsset()))
		{
			RedirectorsToFixArray.Add(RedirectorToFix);
		}
	}

	// AssetTools 모듈을 호출함.
	FAssetToolsModule& AssetToolsModule =
		FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));


	// 호출한 모듈을 가지고 경로를 바꿔줌.
	AssetToolsModule.Get().FixupReferencers(RedirectorsToFixArray);
}

#pragma endregion

#pragma region	CustomEditorTab

void FSuperManagerModule::RegisterAdvancedDeletionTab()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(FName("AdvancedDeletion"),
		FOnSpawnTab::CreateRaw(this, &FSuperManagerModule::OnSpawnAdvanceDeletionTab))
		.SetDisplayName(FText::FromString(TEXT("AdvancedDeletion")));
}

TSharedRef<SDockTab> FSuperManagerModule::OnSpawnAdvanceDeletionTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return
	SNew(SDockTab).TabRole(ETabRole::NomadTab)
		[
			SNew(SAdvanceDeletionTab)
			.AssetsDataToStore(GetAllAssetDataUnderSelectedFolder())
		];
}

TArray<TSharedPtr<FAssetData>> FSuperManagerModule::GetAllAssetDataUnderSelectedFolder()
{
	TArray<TSharedPtr<FAssetData>> AvaliableAssetsData;

	TArray<FString> AssetsPathNames = UEditorAssetLibrary::ListAssets(FolderPathsSelected[0]);

	for (const auto& AssetPathName : AssetsPathNames)
	{
		// Do not touch root Folder!
		// __ExternalActors__ 와 __ExternalObject__ 는 UE5 부터 추가된 파일.
		// Developers, Collections 와 마찬가지로 건들 x
		if (AssetPathName.Contains(TEXT("Developers")) ||
			AssetPathName.Contains(TEXT("Collections")) ||
			AssetPathName.Contains(TEXT("__ExternalActors__")) ||
			AssetPathName.Contains(TEXT("__ExternalObject__")))
		{
			continue;
		}

		if (!UEditorAssetLibrary::DoesAssetExist(AssetPathName)) continue;

		const FAssetData Data =	UEditorAssetLibrary::FindAssetData(AssetPathName);
		
		AvaliableAssetsData.Add(MakeShared<FAssetData>(Data));
	}
	return AvaliableAssetsData;
}

#pragma endregion


#pragma region ProcessDataForAdvanceDeletionTab

bool FSuperManagerModule::DeleteSingleAssetForAssetList(const FAssetData& AssetDataToDelete)
{
	TArray<FAssetData> AssetDataForDeletion;
	AssetDataForDeletion.Add(AssetDataToDelete);

	if (ObjectTools::DeleteAssets(AssetDataForDeletion) > 0)
		return true;
	
	return false;
}

#pragma endregion

void FSuperManagerModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSuperManagerModule, SuperManager)