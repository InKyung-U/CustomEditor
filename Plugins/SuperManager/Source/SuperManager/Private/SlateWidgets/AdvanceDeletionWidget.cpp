// Fill out your copyright notice in the Description page of Project Settings.


#include "SlateWidgets/AdvanceDeletionWidget.h"
#include "SlateBasics.h"
#include "DebugHeader.h"
#include "SuperManager.h"

void SAdvanceDeletionTab::Construct(const FArguments& InArgs)
{
	bCanSupportFocus = true;

	StoreAssetsData = InArgs._AssetsDataToStore;
	FSlateFontInfo TitleTextFont = FCoreStyle::Get().GetFontStyle(FName("EmbossedText"));
	TitleTextFont.Size = 30;

	ChildSlot
	[	// Main Vertical Box
		SNew(SVerticalBox)

		// First vertical slot for title text
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Advance Deletion")))
			.Font(TitleTextFont)
			.Justification(ETextJustify::Center)
			.ColorAndOpacity(FColor::White)
		]

		// 두번째 슬롯
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
		]

		// 세번째 슬롯
		+ SVerticalBox::Slot()
		.VAlign(VAlign_Fill)
		[
			SNew(SScrollBox)

			+ SScrollBox::Slot()
			[
				ConstructAssetListView()
			]
		]

		// 4번째 슬롯
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
		]

	];
}

TSharedRef<SListView<TSharedPtr<FAssetData>>> SAdvanceDeletionTab::ConstructAssetListView()
{
	ConstructedAssetListView =
		SNew(SListView<TSharedPtr<FAssetData>>)
		.ItemHeight(24.f)
		.ListItemsSource(&StoreAssetsData)
		.OnGenerateRow(this, &SAdvanceDeletionTab::OnGenerateRowForList);

	return ConstructedAssetListView.ToSharedRef();
}

TSharedRef<ITableRow> SAdvanceDeletionTab::OnGenerateRowForList(
	TSharedPtr<FAssetData> AssetDataToDisplay, const TSharedRef<STableViewBase>& OwnerTable)
{
	if (!AssetDataToDisplay.IsValid()) 
		return SNew(STableRow<TSharedPtr<FAssetData>>, OwnerTable);

	const FString ClassName = AssetDataToDisplay->GetClass()->GetName();
	const FString ClassName2 = AssetDataToDisplay->GetAsset()->GetClass()->GetName();
	const FString DisplayAssetClassName = AssetDataToDisplay->AssetClass.ToString();
	const FString DisplayAssetName = AssetDataToDisplay->AssetName.ToString();

	FSlateFontInfo AssetClassNameFont = GetEmbossedTextFont();
	AssetClassNameFont.Size = 10;

	FSlateFontInfo AssetNameFont = GetEmbossedTextFont();
	AssetNameFont.Size = 15;

	TSharedRef<STableRow<TSharedPtr<FAssetData>>> ListViewRowWidget =
		SNew(STableRow<TSharedPtr<FAssetData>>, OwnerTable).Padding(FMargin(5.f))
		[
			// 여러 슬롯 추가
			SNew(SHorizontalBox)

			// 1. 체크박스
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.FillWidth(0.05f)
			[
				ConstructCheckBox(AssetDataToDisplay)
			]
						
			// 2. 자산 클래스 이름
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Fill)
			.FillWidth(0.5f)
			[
				ConstructTextForRowWidget(ClassName2, AssetClassNameFont)
			]
			
			// 3. 실제 에셋 이름
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Fill)
			[
				ConstructTextForRowWidget(DisplayAssetName, AssetNameFont)
			]	

			// 4. 삭제 버튼
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Fill)
			.FillWidth(0.2f)
			[
				ConstructButtonForRowWidget(AssetDataToDisplay)
			]
			
		];

	return ListViewRowWidget;
}

TSharedRef<SCheckBox> SAdvanceDeletionTab::ConstructCheckBox(const TSharedPtr<FAssetData>& AssetDataToDisplay)
{
	TSharedRef<SCheckBox> ConstructedCheckBox =
		SNew(SCheckBox)
		.Type(ESlateCheckBoxType::CheckBox)
		.OnCheckStateChanged(this, &SAdvanceDeletionTab::OnCheckBoxStateChanged, AssetDataToDisplay)
		.Visibility(EVisibility::Visible);

	return ConstructedCheckBox;
}

void SAdvanceDeletionTab::OnCheckBoxStateChanged(ECheckBoxState NewState, TSharedPtr<FAssetData> AssetData)
{
	switch (NewState)
	{
	case ECheckBoxState::Unchecked:
		DebugHeader::Print(AssetData->AssetName.ToString() + TEXT(" is unchecked"), FColor::Red);

		break;

	case ECheckBoxState::Checked:
		DebugHeader::Print(AssetData->AssetName.ToString() + TEXT(" is checked"), FColor::Green);

		break;

	case ECheckBoxState::Undetermined:
		break;

	default:
		break;
	}
	
}

TSharedRef<STextBlock> SAdvanceDeletionTab::ConstructTextForRowWidget(const FString& TextContent, const FSlateFontInfo& FontToUse)
{
	TSharedRef<STextBlock> ContstructedTextBlock =
		SNew(STextBlock)
		.Text(FText::FromString(TextContent))
		.Font(FontToUse)
		.ColorAndOpacity(FColor::White);

	return ContstructedTextBlock;
}

TSharedRef<SButton> SAdvanceDeletionTab::ConstructButtonForRowWidget(const TSharedPtr<FAssetData>& AssetDataToDisplay)
{
	TSharedRef<SButton> ConstructButton =
		SNew(SButton)
		.Text(FText::FromString(TEXT("Delete")))
		.OnClicked(this, &SAdvanceDeletionTab::OnDeleteButtonClicked, AssetDataToDisplay);
	
	return ConstructButton;
}

FReply SAdvanceDeletionTab::OnDeleteButtonClicked(TSharedPtr<FAssetData> ClickedAssetData)
{
	FSuperManagerModule& SuperManagerModule = 
		FModuleManager::LoadModuleChecked<FSuperManagerModule>(TEXT("SuperManager"));

	const bool bAssetDeleted = SuperManagerModule.DeleteSingleAssetForAssetList(*ClickedAssetData.Get());

	if (bAssetDeleted)
	{
		// Updating the list source items
		if (StoreAssetsData.Contains(ClickedAssetData))
		{
			StoreAssetsData.Remove(ClickedAssetData);
		}
		// Refresh the list
		RefreshAssetListView();
	}

	return FReply::Handled();
}

void SAdvanceDeletionTab::RefreshAssetListView()
{
	if (ConstructedAssetListView.IsValid())
	{
		ConstructedAssetListView->RebuildList();
	}
}
