// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorActions/QuickActorActionsWidget.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "DebugHeader.h"

void UQuickActorActionsWidget::SelectAllActorWithSimilarName()
{
	if (!GetEditorActorSubsystem()) return;

	TArray<AActor*> SelectedActors = EditorActorSubsystem->GetSelectedLevelActors();
	uint32 SelectionCounter = 0;

	if (SelectedActors.Num() == 0)
	{
		DebugHeader::ShowNotifyInfo(TEXT("No Actor Selected."));
		return;
	}

	if (SelectedActors.Num() > 1)
	{
		DebugHeader::ShowNotifyInfo(TEXT("You can onlt Select one actor."));
		return;
	}

	FString SelectedActorName = SelectedActors[0]->GetActorLabel();
	const FString NameToSearch = SelectedActorName.LeftChop(4);

	TArray<AActor*> AllLevelActors = EditorActorSubsystem->GetAllLevelActors();

	for (auto Actor : AllLevelActors)
	{
		if (!Actor) continue;

		if (Actor->GetActorLabel().Contains(NameToSearch, SearchCase))
		{
			EditorActorSubsystem->SetActorSelectionState(Actor, true);
			SelectionCounter++;
		}
	}

	if (SelectionCounter > 0)
	{
		DebugHeader::ShowNotifyInfo(TEXT("Successfully selected ") + FString::FromInt(SelectionCounter)
		+ TEXT(" actors."));
	}
	else
	{
		DebugHeader::ShowNotifyInfo(TEXT("No Actor with similar name found."));

	}
}

void UQuickActorActionsWidget::DuplicateActors()
{
	if (!GetEditorActorSubsystem()) return;

	TArray<AActor*> SelectedActors = EditorActorSubsystem->GetSelectedLevelActors();
	uint32 Counter = 0;

	if (SelectedActors.Num() == 0)
	{
		DebugHeader::ShowNotifyInfo(TEXT("No Actor Selected."));
		return;
	}

	if (NumberOfDuplicates <= 0 || OffsetDist == 0.f)
	{
		DebugHeader::ShowNotifyInfo(TEXT("Did not specift a number of duplications or an offset distance"));
		return;
	}

	for (auto SelectedActor : SelectedActors)
	{
		if (!SelectedActor) continue;

		for (int32 i = 0; i < NumberOfDuplicates; i++)
		{
			AActor* DuplicatedActor =
				EditorActorSubsystem->DuplicateActor(SelectedActor, SelectedActor->GetWorld());

			if (!DuplicatedActor) continue;

			const float DuplicationOffsetDist = (i + 1) * OffsetDist;

			switch (AxisForDuplication)
			{
			case E_DuplicationAxis::EDA_XAxis:
				DuplicatedActor->AddActorWorldOffset(FVector(DuplicationOffsetDist, 0.f, 0.f));
				break;

			case E_DuplicationAxis::EDA_YAxis:
				DuplicatedActor->AddActorWorldOffset(FVector(0.f, DuplicationOffsetDist, 0.f));
				break;

			case E_DuplicationAxis::EDA_ZAxis:
				DuplicatedActor->AddActorWorldOffset(FVector(0.f, 0.f, DuplicationOffsetDist));
				break;

			case E_DuplicationAxis::EDA_MAX:
				break;
			default:
				break;
			}

			EditorActorSubsystem->SetActorSelectionState(DuplicatedActor, true);
			Counter++;
		}
	
	}

	if (Counter > 0)
	{
		DebugHeader::ShowNotifyInfo(TEXT("Successfully Duplicated ") + FString::FromInt(Counter)
			+ TEXT(" actors."));

	}
}

void UQuickActorActionsWidget::RandomizeActorTransform()
{
	const bool bConditionNotSet = 
		!RandomActorRotation.bRandomizeRotYaw && 
		!RandomActorRotation.bRandomizeRotPitch && 
		!RandomActorRotation.bRandomizeRotRoll&&
		!bRandomizeScale && !bRandomizeOffset ;

	if (bConditionNotSet)
	{
		DebugHeader::ShowNotifyInfo(TEXT("No variation condition specified."));
		return;
	}

	if (!GetEditorActorSubsystem()) return;

	TArray<AActor*> SelectedActors = EditorActorSubsystem->GetSelectedLevelActors();
	uint32 Counter = 0;

	if (SelectedActors.Num() == 0)
	{
		DebugHeader::ShowNotifyInfo(TEXT("No Actor Selected."));
		return;
	}

	for (auto SelectedActor : SelectedActors)
	{
		if (!SelectedActor) continue;

		//FRotator ActorRotation = SelectedActor->GetActorRotation();
		//ActorRotation.Yaw += randRange;
		//SelectedActor->SetActorRotation(ActorRotation);

		if(RandomActorRotation.bRandomizeRotYaw)
		{
			const float randRangeYaw = (float)FMath::RandRange(RandomActorRotation.RotYawMin, RandomActorRotation.RotYawMax);
			SelectedActor->AddActorWorldRotation(FRotator(0.f, randRangeYaw, 0.f));
		}

		if (RandomActorRotation.bRandomizeRotPitch)
		{
			const float randRangePitch = (float)FMath::RandRange(RandomActorRotation.RotPitchMin, RandomActorRotation.RotPitchMax);
			SelectedActor->AddActorWorldRotation(FRotator(randRangePitch, 0.f, 0.f));
		}

		if (RandomActorRotation.bRandomizeRotRoll)
		{
			const float randRangeYRoll = (float)FMath::RandRange(RandomActorRotation.RotRollMin, RandomActorRotation.RotRollMax);
			SelectedActor->AddActorWorldRotation(FRotator(0.f, 0.f, randRangeYRoll));
		}

		if (bRandomizeScale)
		{
			const float randSacleValue = (float)FMath::RandRange(ScaleMin, ScaleMax);
			SelectedActor->SetActorScale3D(FVector(randSacleValue));
		}

		if (bRandomizeOffset)
		{
			const float randOffsetValueX = (float)FMath::RandRange(OffsetMin, OffsetMax);
			const float randOffsetValueY = (float)FMath::RandRange(OffsetMin, OffsetMax);
			const float randOffsetValueZ = (float)FMath::RandRange(OffsetMin, OffsetMax);

			SelectedActor->AddActorWorldOffset(FVector(randOffsetValueX, randOffsetValueY, randOffsetValueZ));
		}

		Counter++;
	}

	if (Counter > 0)
	{
		DebugHeader::ShowNotifyInfo(TEXT("Successfully set ") + FString::FromInt(Counter)
			+ TEXT(" actors."));
	}
}

bool UQuickActorActionsWidget::GetEditorActorSubsystem()
{
	if (!EditorActorSubsystem)
	{
		EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
	}

	return EditorActorSubsystem != nullptr;
}
