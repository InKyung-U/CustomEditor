// Fill out your copyright notice in the Description page of Project Settings.


#include "AssetActions/QuickMaterialCreationWidget.h"
#include "DebugHeader.h"
#include "EditorUtilityLibrary.h"
#include "EditorAssetLibrary.h"
#include "AssetToolsModule.h"
#include "Factories/MaterialFactoryNew.h"

#pragma region QuickMaterialCreationCore

void UQuickMaterialCreationWidget::CreateMaterialFromSelectedTexture()
{
	if (bCustomMaterialName)
	{
		if (MaterialName.IsEmpty() || MaterialName.Equals(TEXT("M_")))
		{
			DebugHeader::ShowMsgDialog(EAppMsgType::Ok, TEXT("Please enter a valid name"));
		}
	}

	TArray<FAssetData> SelectedAssetData = UEditorUtilityLibrary::GetSelectedAssetData();
	TArray<UTexture2D*> SelectedTexturesArray;
	FString SelectedTextureFolderPath;

	if (!ProcessSelectedData(SelectedAssetData, SelectedTexturesArray, SelectedTextureFolderPath))
		return;

	if (CheckIsNameUsed(SelectedTextureFolderPath, MaterialName))
		return;
	UMaterial* CreatedMaterial = CreateMaterialAssets(MaterialName, SelectedTextureFolderPath);

	if (!CreatedMaterial)
	{
		DebugHeader::ShowMsgDialog(EAppMsgType::Ok, TEXT("Failed to Created Material"));
		return;
	}
}
#pragma endregion


#pragma region QuickMaterialCreation

// �ؽ�ó�� ���͸��ϴ� �Լ�.
bool UQuickMaterialCreationWidget::ProcessSelectedData(
	const TArray<FAssetData>& SelectedDataToProcess,
	TArray<UTexture2D*>& OutSelectedTexturesArray, 
	FString& OutSelectedTexturePackagePath)
{
	if (SelectedDataToProcess.Num() == 0)
	{
		DebugHeader::ShowMsgDialog(EAppMsgType::Ok, TEXT("No Texture Selected!"));
		return false;
	}

	bool bMaterialNameSet = false;
	for (const auto& SelectedData : SelectedDataToProcess)
	{
		UObject* SelectedAsset = SelectedData.GetAsset();

		if (!SelectedAsset)
			continue;

		UTexture2D* SelectedTexture = Cast<UTexture2D>(SelectedAsset);

		if (!SelectedTexture)
		{
			DebugHeader::ShowMsgDialog(EAppMsgType::Ok, TEXT("Please Selected Only Textures!\n") +
				SelectedAsset->GetName() + TEXT("  is Not Textures..."));
			return false;
		}

		OutSelectedTexturesArray.Add(SelectedTexture);

		if(OutSelectedTexturePackagePath.IsEmpty())
			OutSelectedTexturePackagePath = SelectedData.PackagePath.ToString();

		// ����ڰ� �ؽ�ó �̸��� ������ ����ϱ⸦ ����.
		if (!bCustomMaterialName && !bMaterialNameSet)
		{
			bMaterialNameSet = true;
			MaterialName = SelectedAsset->GetName();
			MaterialName.RemoveFromStart(TEXT("T_"));
			MaterialName.InsertAt(0, TEXT("M_"));
		}
	}
	return true;
}

bool UQuickMaterialCreationWidget::CheckIsNameUsed(
	const FString& FolderPathToCheck, const FString& MaterialNameToCheck)
{
	// �Ʒ� �Լ� �� ��° ���ڴ� ��������� �� ���̳�. ���ο� �ִ� �������� �˻��� ���̳� ������.
	TArray<FString> ExistingAssetsPaths = UEditorAssetLibrary::ListAssets(FolderPathToCheck, false);

	for (const auto& ExistingAssetPath : ExistingAssetsPaths)
	{
		const FString ExistingAssetName =  FPaths::GetBaseFilename(ExistingAssetPath);

		if (ExistingAssetName.Equals(MaterialNameToCheck))
		{
			DebugHeader::ShowMsgDialog(EAppMsgType::Ok, 
				MaterialNameToCheck + TEXT(" is already used by asset."));
			return true;
		}
	}
	return false;
}

UMaterial* UQuickMaterialCreationWidget::CreateMaterialAssets(
	const FString& NameOfTheMaterial, const FString& PathtoPutMaterial)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	
	UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

	UObject* CreatedObject = AssetToolsModule.Get().CreateAsset(NameOfTheMaterial, PathtoPutMaterial,
		UMaterial::StaticClass(), MaterialFactory);

	return Cast<UMaterial>(CreatedObject);
}

#pragma endregion