// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "WayPoint.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/Character.h"
#include "KarenCharacter.generated.h"



UENUM(BlueprintType)
enum class ENPCState : uint8{
	Idle UMETA(DisplayName = "Idle"),
	Patrol UMETA(DisplayName = "Patrol"),
	Chase UMETA(DisplayName = "Chase"),
	Attack UMETA(DisplayName = "Attack"),
	Wait UMETA(DisplayName = "Wait")
};

UCLASS()
class GROCERYRUN_API AKarenCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AKarenCharacter();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior")
	TArray<AWayPoint*> PatrolPoints;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior")
	float RunSpeed = 1.1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation Time")
	float MaxIdleAnimTransitionTime = .3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation Time")
	float MaxPatrolAnimTransitionTime = .3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior")
	float MaxWaitTime = 3;
	UPROPERTY(BlueprintReadOnly, Category="Behavior")
	float WalkingValue = 0;
	UPROPERTY(BlueprintReadOnly, Category="Behavior")
	bool AnimTransitioning = false;

	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void TransitionToPatrolAnimation(float DeltaTime);
	void TransitionToIdleAnimation(float DeltaTime);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void MoveToNextPatrolPoint();

	void WaitBeforeMove(float DeltaTime);

	int ChooseRandomPatrolPoint();

	UFUNCTION()
	void OnMoveCompletedHandler(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	UFUNCTION(BlueprintCallable)
	ENPCState GetCurrentMoveState(){ return CurrentMoveState;}

private:
	int LastPatrolPoint = 0;
	bool IsPowerWalking = false;
	bool ShouldWaitNextMove = false;
	AAIController* AiController = nullptr;
	ENPCState CurrentMoveState = ENPCState::Idle;
	float Timer = 0;
	float WaitTimer = 0;
};


