// Copyright Epic Games, Inc. All Rights Reserved.


#include "GroceryRunPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "KarenCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"

void AGroceryRunPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// get the enhanced input subsystem
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// add the mapping context so we get controls
		Subsystem->AddMappingContext(InputMappingContext, 0);
	}
}

void AGroceryRunPlayerController::DebugTransitionTrigger(FName CharacterTag, FString num) const
{

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), CharacterTag, FoundActors);
	AKarenCharacter* KarenCharacter = nullptr;
	for(AActor* ac : FoundActors)
	{
		if(AKarenCharacter* temp = Cast<AKarenCharacter>(ac))
		{
			KarenCharacter = temp;
			break;
		}
		UE_LOG(LogTemp, Warning, TEXT("Actor Found"));

	}

	if(!KarenCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Debug Karen Character"));

		return;
	}
	if(num == "one")
	{
		KarenCharacter->TransitionTriggerFirst = !KarenCharacter->TransitionTriggerFirst;
		UE_LOG(LogTemp, Warning, TEXT("Setting Transition First To %s"), KarenCharacter->TransitionTriggerFirst ? TEXT("True") : TEXT("False"));
	}
	else if(num == "two")
	{
		KarenCharacter->TransitionTriggerSecond = !KarenCharacter->TransitionTriggerSecond;
		UE_LOG(LogTemp, Warning, TEXT("Setting Transition Second To %s"), KarenCharacter->TransitionTriggerSecond ? TEXT("True") : TEXT("False"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Unrecognized Command"));

	}
}
