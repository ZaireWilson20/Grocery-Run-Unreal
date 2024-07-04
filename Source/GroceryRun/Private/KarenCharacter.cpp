// Fill out your copyright notice in the Description page of Project Settings.


#include "GroceryRun/Public/KarenCharacter.h"

#include "AIController.h"
#include "VectorTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"

class AAIController;


// Sets default values
AKarenCharacter::AKarenCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	CurrentMoveState = ENPCState::Idle;
}

// Called when the game starts or when spawned
void AKarenCharacter::BeginPlay()
{
	Super::BeginPlay();

	if(AiController == nullptr){
		AiController = Cast<AAIController>(GetController());
		AiController->ReceiveMoveCompleted.AddDynamic(this, &AKarenCharacter::OnMoveCompletedHandler);

	}
	
}

void AKarenCharacter::TransitionToPatrolAnimation(float DeltaTime){
	Timer += DeltaTime;
	
	float alpha = FMath::Clamp(Timer/MaxPatrolAnimTransitionTime, 0.0, 1.0);
	WalkingValue = FMath::Lerp(0.0, 1.0, alpha);

	if(Timer >= MaxPatrolAnimTransitionTime){
		AnimTransitioning = false;
		Timer = 0; 
	}


}

void AKarenCharacter::TransitionToIdleAnimation(float DeltaTime){
	Timer += DeltaTime;
	
	float alpha = FMath::Clamp(Timer/MaxIdleAnimTransitionTime, 0.0, 1.0);
	WalkingValue = FMath::Lerp(1.0, 0.0, alpha);

	if(Timer >= MaxIdleAnimTransitionTime){
		AnimTransitioning = false;
		Timer = 0; 
	}
}

// Called every frame
void AKarenCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(AnimTransitioning){
		switch (CurrentMoveState)
		{
		case ENPCState::Idle:
			TransitionToIdleAnimation(DeltaTime);
			break;
		case ENPCState::Patrol:
			TransitionToPatrolAnimation(DeltaTime);
			break;
		}	

	}
	
	
	if(PatrolPoints.Num() > 0){
		if(AiController == nullptr || (CurrentMoveState == ENPCState::Idle && !AnimTransitioning && !ShouldWaitNextMove)){
			MoveToNextPatrolPoint();
		} else if (ShouldWaitNextMove) {
			WaitBeforeMove(DeltaTime);
		}
	}
	
}

// Called to bind functionality to input
void AKarenCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AKarenCharacter::OnMoveCompletedHandler(FAIRequestID RequestID, EPathFollowingResult::Type Result){
	if (Result == EPathFollowingResult::Success){
		UE_LOG(LogTemp, Warning, TEXT("Move completed successfully!"));
	} else if (Result == EPathFollowingResult::Blocked) {
		UE_LOG(LogTemp, Warning, TEXT("Move blocked!"));
	} else if (Result == EPathFollowingResult::OffPath) {
		UE_LOG(LogTemp, Warning, TEXT("Move off path!"));
	} else if (Result == EPathFollowingResult::Aborted) {
		UE_LOG(LogTemp, Warning, TEXT("Move aborted!"));
	}
	AnimTransitioning = true;
	ShouldWaitNextMove = true;
	WaitTimer = 0;
	CurrentMoveState = ENPCState::Idle;
}

void AKarenCharacter::WaitBeforeMove(float DeltaTime){
	WaitTimer += DeltaTime;
	
	if(WaitTimer >= MaxWaitTime){
		WaitTimer = 0;
		ShouldWaitNextMove = false;
		
	}
}

void AKarenCharacter::MoveToNextPatrolPoint(){
	CurrentMoveState = ENPCState::Patrol;
	LastPatrolPoint = ChooseRandomPatrolPoint();
	AnimTransitioning = true;
	if(AiController == nullptr){
		AiController = Cast<AAIController>(GetController()->GetPawn()->GetController());
	}

	AiController->MoveToLocation(PatrolPoints[LastPatrolPoint]->GetActorLocation());
	UE_LOG(LogTemp, Warning, TEXT("Move started!"));
	IsPowerWalking = FMath::RandRange(0,1) == 1;
	
	if(IsPowerWalking){
		GetCharacterMovement()->Velocity *= RunSpeed;
	}
}

int AKarenCharacter::ChooseRandomPatrolPoint(){
	int minValue = 0;
	int maxValue = PatrolPoints.Num();
	int randomIntInRange = FMath::Clamp(FMath::RandRange(minValue, maxValue - 1), 0, maxValue - 1);

	if(randomIntInRange == LastPatrolPoint && maxValue != 1){
		if(randomIntInRange == maxValue - 1){
			randomIntInRange -= 1;
		} else {
			randomIntInRange += 1;
		}
	}

	return randomIntInRange;
}

float HorizontalDistance(FVector a, FVector b){
	return UE::Geometry::Distance(FVector(a.X, 0, a.Z), FVector(a.X, 0, a.Z));
}



