// Fill out your copyright notice in the Description page of Project Settings.


#include "AssetActions/QuickAssetAction.h"
#include "DebugHeader.h"
#include "EditorUtilityLibrary.h"
#include "EditorAssetLibrary.h"
#include "ObjectTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"

void UQuickAssetAction::DuplicateAsset(int32 NumOfDuplicates)
{
	if (NumOfDuplicates <= 0)
	{		
		ShowMsgDialog(EAppMsgType::Ok, TEXT("Please enter a VALID number"));
		return;
	}

	TArray<FAssetData> SelectedAssetsData = UEditorUtilityLibrary::GetSelectedAssetData();
	uint32 Counter = 0;

	for (const auto& SelectedAssetData : SelectedAssetsData)
	{
		for (int32 i = 0; i < NumOfDuplicates; i++)
		{
			// ������ ���
			//const FString SourceAssetPath = SelectedAssetData.ObjectPath.ToString();
			const FString SourceAssetPath = SelectedAssetData.GetSoftObjectPath().ToString();

			// ����ǰ�� "�̸� + _����Index" �� ������ش�.
			const FString NewDuplicatedAssetName = SelectedAssetData.AssetName.ToString() 
													+ TEXT("_") + FString::FromInt(i+1);
			
			// ������ ������ ������ ���. ������ ������ ��Ű�� ��� �� ���� ���� ����ǰ�� �̸��� �޾��־�� ��.
			const FString NewPathName 
				= FPaths::Combine(SelectedAssetData.PackagePath.ToString(), NewDuplicatedAssetName);

			if (UEditorAssetLibrary::DuplicateAsset(SourceAssetPath, NewPathName))
			{
				UEditorAssetLibrary::SaveAsset(NewPathName, false);
				++Counter;
			}
		}
	}

	if(Counter > 0)
	{
		ShowNotifyInfo(TEXT("Successfully duplicated " + FString::FromInt(Counter) + "files"));
	}

}

void UQuickAssetAction::AddPrefixes()
{
	TArray<UObject*> SelectedObjects = UEditorUtilityLibrary::GetSelectedAssets();
	uint32 Counter = 0;

	for (auto SelectedObject : SelectedObjects)
	{
		if (!SelectedObject) continue;

		FString* PrefixFound = PrefixMap.Find(SelectedObject->GetClass());

		if (!PrefixFound || PrefixFound->IsEmpty())
		{
			Print(TEXT("Failed to find prefix for class") + SelectedObject->GetClass()->GetName(), FColor::Red);
			continue;
		}

		FString OldName = SelectedObject->GetName();
		if (OldName.StartsWith(*PrefixFound))
		{
			Print(OldName + TEXT(" already has prefix added"), FColor::Red);
			continue;
		}

		if (SelectedObject->IsA<UMaterialInstanceConstant>())
		{
			OldName.RemoveFromStart(TEXT("M_"));
			OldName.RemoveFromEnd(TEXT("_Inst"));
		}

		const FString NewNameWithPrefix = *PrefixFound + OldName;

		UEditorUtilityLibrary::RenameAsset(SelectedObject, NewNameWithPrefix);
		Counter++;
	}

	ShowNotifyInfo(TEXT("Successfully renamed " + FString::FromInt(Counter) + " assets"));
}

void UQuickAssetAction::RemoveUnusedAssets()
{
	TArray<FAssetData> SelectedAssetsData = UEditorUtilityLibrary::GetSelectedAssetData();
	TArray<FAssetData> UnusedAssetsData;

	// �����ϱ����� ������ �������� �������ش�.
	FixUpRedirectors();

	for (const auto& SelectedAssetData : SelectedAssetsData)
	{
		TArray<FString> AssetRefrencers = 
		UEditorAssetLibrary::FindPackageReferencersForAsset(SelectedAssetData.GetSoftObjectPath().ToString());

		if (AssetRefrencers.Num() == 0)
		{
			UnusedAssetsData.Add(SelectedAssetData);
		}
	}

	if (UnusedAssetsData.Num() == 0)
	{
		ShowMsgDialog(EAppMsgType::Ok, TEXT("No Unused asset found among selected assets"), false);
		return;
	}

	const int32 NumOfAssetsDeleted = ObjectTools::DeleteAssets(UnusedAssetsData);
	if (NumOfAssetsDeleted == 0) return;
	ShowNotifyInfo(TEXT("Succecfully deleted " + FString::FromInt(NumOfAssetsDeleted) + TEXT(" UnusedAssets")));
}

void UQuickAssetAction::FixUpRedirectors()
{
	TArray<UObjectRedirector*> RedirectorsToFixArray;

	FAssetRegistryModule& AssetRegistryModule = 
		FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace("/Game");
	Filter.ClassNames.Emplace("ObjectRedirector");

	TArray<FAssetData> OutRedirectors;
	AssetRegistryModule.Get().GetAssets(Filter, OutRedirectors);

	for (const auto& RedirectorData : OutRedirectors)
	{
		if (UObjectRedirector* RedirectorToFix = Cast<UObjectRedirector>(RedirectorData.GetAsset()))
		{
			RedirectorsToFixArray.Add(RedirectorToFix);
		}
	}

	FAssetToolsModule& AssetToolsModule = 
		FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));

	AssetToolsModule.Get().FixupReferencers(RedirectorsToFixArray);

}
