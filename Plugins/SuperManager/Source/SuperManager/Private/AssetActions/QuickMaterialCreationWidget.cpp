// Fill out your copyright notice in the Description page of Project Settings.


#include "AssetActions/QuickMaterialCreationWidget.h"
#include "DebugHeader.h"
#include "EditorUtilityLibrary.h"
#include "EditorAssetLibrary.h"
#include "AssetToolsModule.h"
#include "Factories/MaterialFactoryNew.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"

#pragma region QuickMaterialCreationCore

void UQuickMaterialCreationWidget::CreateMaterialFromSelectedTexture()
{
	if (bCustomMaterialName)
	{
		if (MaterialName.IsEmpty() || MaterialName.Equals(TEXT("M_")))
		{
			DebugHeader::ShowMsgDialog(EAppMsgType::Ok, TEXT("Please enter a valid name"));
			return;
		}
	}

	TArray<FAssetData> SelectedAssetData = UEditorUtilityLibrary::GetSelectedAssetData();
	TArray<UTexture2D*> SelectedTexturesArray;
	FString SelectedTextureFolderPath;
	uint32 PinsConnectedCounter = 0;

	if (!ProcessSelectedData(SelectedAssetData, SelectedTexturesArray, SelectedTextureFolderPath))
	{
		MaterialName = TEXT("M_");
		return;
	}

	if (CheckIsNameUsed(SelectedTextureFolderPath, MaterialName))
	{
		MaterialName = TEXT("M_");
		return;
	}

	UMaterial* CreatedMaterial = CreateMaterialAssets(MaterialName, SelectedTextureFolderPath);

	if (!CreatedMaterial)
	{
		DebugHeader::ShowMsgDialog(EAppMsgType::Ok, TEXT("Failed to Created Material"));
		return;
	}

	for (auto SelectedTexture : SelectedTexturesArray)
	{
		if (!SelectedTexture)
			continue;
		 
		switch (ChannelPackingType)
		{
		case E_ChannelPackingType::ECPT_NoChannelPacking:
			Default_CreateMaterialNodes(CreatedMaterial, SelectedTexture, PinsConnectedCounter);
			break;

		case E_ChannelPackingType::ECPT_ORM:
			ORM_CreateMaterialNodes(CreatedMaterial, SelectedTexture, PinsConnectedCounter);
			break;
		case E_ChannelPackingType::ECPT_MAX:
			break;
		default:
			break;
		}
		
	}

	if (PinsConnectedCounter > 0)
	{
		DebugHeader::ShowNotifyInfo(TEXT("Successfully connected ") + FString::FromInt(PinsConnectedCounter)
		+ TEXT(" pins."));
	}

	if (bCreateMaterialInstance)
	{
		CreateMaterialInstanceAsset(CreatedMaterial, MaterialName, SelectedTextureFolderPath);
	}

	MaterialName = TEXT("M_");
}
#pragma endregion


#pragma region QuickMaterialCreation

// 텍스처를 필터링하는 함수.
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

		// 사용자가 텍스처 이름을 재질로 사용하기를 원함.
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
	// 아래 함수 두 번째 인자는 재귀적으로 할 것이냐. 내부에 있는 폴더까지 검사할 것이냐 여부임.
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

void UQuickMaterialCreationWidget::Default_CreateMaterialNodes(
	UMaterial* CreatedMaterial, UTexture2D* SelectedTexture, uint32& PinsConnectedCounter)
{
	UMaterialExpressionTextureSample* TextureSampleNode = NewObject<UMaterialExpressionTextureSample>(CreatedMaterial);
	
	if (!TextureSampleNode) return;

	// 핀 연결 가능
	if (!CreatedMaterial->HasBaseColorConnected())
	{
		if (TryConnectBaseColor(TextureSampleNode, SelectedTexture, CreatedMaterial))
		{
			PinsConnectedCounter++;
			return;
		}
	}

	if (!CreatedMaterial->HasMetallicConnected())
	{
		if (TryConnectMetalic(TextureSampleNode, SelectedTexture, CreatedMaterial))
		{
			PinsConnectedCounter++;
			return;
		}
	}

	if (!CreatedMaterial->HasRoughnessConnected())
	{
		if (TryConnectRoughness(TextureSampleNode, SelectedTexture, CreatedMaterial))
		{
			PinsConnectedCounter++;
			return;
		}
	}

	if (!CreatedMaterial->HasNormalConnected())
	{
		if (TryConnectNormal(TextureSampleNode, SelectedTexture, CreatedMaterial))
		{
			PinsConnectedCounter++;
			return;
		}
	}

	if (!CreatedMaterial->HasAmbientOcclusionConnected())
	{
		if (TryConnectAO(TextureSampleNode, SelectedTexture, CreatedMaterial))
		{
			PinsConnectedCounter++;
			return;
		}
	}

}

void UQuickMaterialCreationWidget::ORM_CreateMaterialNodes(UMaterial* CreatedMaterial, UTexture2D* SelectedTexture, uint32& PinsConnectedCounter)
{
	// 베이스 컬러, 노멀, ARM 만 연결.
	UMaterialExpressionTextureSample* TextureSampleNode = NewObject<UMaterialExpressionTextureSample>(CreatedMaterial);

	if (!TextureSampleNode) return;

	// 핀 연결 가능
	if (!CreatedMaterial->HasBaseColorConnected())
	{
		if (TryConnectBaseColor(TextureSampleNode, SelectedTexture, CreatedMaterial))
		{
			PinsConnectedCounter++;
			return;
		}
	}

	if (!CreatedMaterial->HasNormalConnected())
	{
		if (TryConnectNormal(TextureSampleNode, SelectedTexture, CreatedMaterial))
		{
			PinsConnectedCounter++;
			return;
		}
	}

	if (!CreatedMaterial->HasRoughnessConnected())
	{
		if (TryConnectORM(TextureSampleNode, SelectedTexture, CreatedMaterial))
		{
			PinsConnectedCounter += 3;
			return;
		}
	}
}
#pragma endregion


#pragma region CreateMaterialNodesConnectPins

bool UQuickMaterialCreationWidget::TryConnectBaseColor(
	UMaterialExpressionTextureSample* TextureSampleNode, UTexture2D* SelectedTexture, UMaterial* CreatedMaterial)
{
	for (const auto& BaseColorName : BaseColorArray)
	{
		if (SelectedTexture->GetName().Contains(BaseColorName))
		{
			// 베이스컬러 텍스처를 여기서 이어준다.
			TextureSampleNode->Texture = SelectedTexture;
			// UE 5.1.1 부터 아래 코드 불가.
			//CreatedMaterial->Expression.Add(TextureSampleNode);
			// 아래와 같이 사용할 것.
			CreatedMaterial->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(TextureSampleNode);
			CreatedMaterial->GetEditorOnlyData()->BaseColor.Expression = TextureSampleNode;
			CreatedMaterial->GetEditorOnlyData()->PostEditChange();

			TextureSampleNode->MaterialExpressionEditorX -= 600;

			return true;
		}
	}

	return false;
}

bool UQuickMaterialCreationWidget::TryConnectMetalic(
	UMaterialExpressionTextureSample* TextureSampleNode, 
	UTexture2D* SelectedTexture, UMaterial* CreatedMaterial)
{
	for (const auto& MetalicName : MetallicArray)
	{
		if (SelectedTexture->GetName().Contains(MetalicName))
		{
			// 메탈릭 텍스처를 여기서 이어준다.
			SelectedTexture->CompressionSettings = TextureCompressionSettings::TC_Default;
			SelectedTexture->SRGB = false;
			SelectedTexture->PostEditChange();

			TextureSampleNode->Texture = SelectedTexture;
			TextureSampleNode->SamplerType = EMaterialSamplerType::SAMPLERTYPE_LinearColor;

			CreatedMaterial->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(TextureSampleNode);
			CreatedMaterial->GetEditorOnlyData()->Metallic.Expression = TextureSampleNode;
			CreatedMaterial->GetEditorOnlyData()->PostEditChange();

			TextureSampleNode->MaterialExpressionEditorX -= 600;
			TextureSampleNode->MaterialExpressionEditorY += 240;

			return true;
		}
	}

	return false;
}

bool UQuickMaterialCreationWidget::TryConnectRoughness(UMaterialExpressionTextureSample* TextureSampleNode, UTexture2D* SelectedTexture, UMaterial* CreatedMaterial)
{
	for (const auto& RoughnessName : RoughnessArray)
	{
		if (SelectedTexture->GetName().Contains(RoughnessName))
		{
			// 메탈릭 텍스처를 여기서 이어준다.
			SelectedTexture->CompressionSettings = TextureCompressionSettings::TC_Default;
			SelectedTexture->SRGB = false;
			SelectedTexture->PostEditChange();

			TextureSampleNode->Texture = SelectedTexture;
			TextureSampleNode->SamplerType = EMaterialSamplerType::SAMPLERTYPE_LinearColor;

			CreatedMaterial->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(TextureSampleNode);
			CreatedMaterial->GetEditorOnlyData()->Roughness.Expression = TextureSampleNode;
			CreatedMaterial->GetEditorOnlyData()->PostEditChange();

			TextureSampleNode->MaterialExpressionEditorX -= 600;
			TextureSampleNode->MaterialExpressionEditorY += 240*2;

			return true;
		}
	}
	return false;
}

bool UQuickMaterialCreationWidget::TryConnectNormal(UMaterialExpressionTextureSample* TextureSampleNode, UTexture2D* SelectedTexture, UMaterial* CreatedMaterial)
{
	for (const auto& NormalName : NormalArray)
	{
		if (SelectedTexture->GetName().Contains(NormalName))
		{
			TextureSampleNode->Texture = SelectedTexture;
			TextureSampleNode->SamplerType = EMaterialSamplerType::SAMPLERTYPE_Normal;

			CreatedMaterial->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(TextureSampleNode);
			CreatedMaterial->GetEditorOnlyData()->Normal.Expression = TextureSampleNode;
			CreatedMaterial->GetEditorOnlyData()->PostEditChange();

			TextureSampleNode->MaterialExpressionEditorX -= 600;
			TextureSampleNode->MaterialExpressionEditorY += 240*3;

			return true;
		}
	}
	return false;
}

bool UQuickMaterialCreationWidget::TryConnectAO(UMaterialExpressionTextureSample* TextureSampleNode, UTexture2D* SelectedTexture, UMaterial* CreatedMaterial)
{
	for (const auto& AOName : AmbientOcclusionArray)
	{
		if (SelectedTexture->GetName().Contains(AOName))
		{
			// 메탈릭 텍스처를 여기서 이어준다.
			SelectedTexture->CompressionSettings = TextureCompressionSettings::TC_Default;
			SelectedTexture->SRGB = false;
			SelectedTexture->PostEditChange();

			TextureSampleNode->Texture = SelectedTexture;
			TextureSampleNode->SamplerType = EMaterialSamplerType::SAMPLERTYPE_LinearColor;

			CreatedMaterial->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(TextureSampleNode);
			CreatedMaterial->GetEditorOnlyData()->AmbientOcclusion.Expression = TextureSampleNode;
			CreatedMaterial->GetEditorOnlyData()->PostEditChange();

			TextureSampleNode->MaterialExpressionEditorX -= 600;
			TextureSampleNode->MaterialExpressionEditorY += 240*4;

			return true;
		}
	}
	return false;
}

bool UQuickMaterialCreationWidget::TryConnectORM(UMaterialExpressionTextureSample* TextureSampleNode, UTexture2D* SelectedTexture, UMaterial* CreatedMaterial)
{
	for (const auto& ORM_Name : ORMArray)
	{
		if (SelectedTexture->GetName().Contains(ORM_Name))
		{
			// 메탈릭 텍스처를 여기서 이어준다.
			SelectedTexture->CompressionSettings = TextureCompressionSettings::TC_Masks;
			SelectedTexture->SRGB = false;
			SelectedTexture->PostEditChange();

			TextureSampleNode->Texture = SelectedTexture;
			TextureSampleNode->SamplerType = EMaterialSamplerType::SAMPLERTYPE_Masks;

			CreatedMaterial->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(TextureSampleNode);
			CreatedMaterial->GetEditorOnlyData()->AmbientOcclusion.Connect(1, TextureSampleNode);
			CreatedMaterial->GetEditorOnlyData()->Roughness.Connect(2, TextureSampleNode);
			CreatedMaterial->GetEditorOnlyData()->Metallic.Connect(3, TextureSampleNode);
			CreatedMaterial->GetEditorOnlyData()->PostEditChange();

			TextureSampleNode->MaterialExpressionEditorX -= 600;
			TextureSampleNode->MaterialExpressionEditorY += 240 * 4;

			return true;
		}
	}

	return false;
}

#pragma endregion

UMaterialInstanceConstant* UQuickMaterialCreationWidget::CreateMaterialInstanceAsset(
	UMaterial* CreatedMaterial, FString NameOfMaterialInstance, const FString& PathToPutMaterial)
{
	NameOfMaterialInstance.RemoveFromStart(TEXT("M_"));
	NameOfMaterialInstance.InsertAt(0, TEXT("MI_"));

	UMaterialInstanceConstantFactoryNew* MIFactoryNew = NewObject<UMaterialInstanceConstantFactoryNew>();

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));

	UObject* CreatedObject = AssetToolsModule.Get().CreateAsset(
		NameOfMaterialInstance, PathToPutMaterial, UMaterialInstanceConstant::StaticClass(), MIFactoryNew);

	if (UMaterialInstanceConstant* CreatedMI = Cast<UMaterialInstanceConstant>(CreatedObject))
	{
		CreatedMI->SetParentEditorOnly(CreatedMaterial);
		CreatedMI->PostEditChange();
		CreatedMaterial->PostEditChange();

		return CreatedMI;
	}

	return nullptr;
}
