// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "WayPoint.h"
#include "GR_Enums.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/Character.h"
#include "GroceryRun/GroceryRunCharacter.h"
#include "KarenCharacter.generated.h"





UCLASS()
class GROCERYRUN_API AKarenCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AKarenCharacter();


	//TODO: Just use one curve when linear time is needed
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Timing")
	UCurveFloat* PatienceTimeCurve;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Timing")
	UCurveFloat* LinearMinuteTimeCurve;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Timing")
	float TimeToTriggerChase = 3.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Timing")
	float TimeToTriggerExitChase = 3.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Timing")
	float TimeToTriggerCrashOut = 3.0f;
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
	UPROPERTY(BlueprintReadOnly, Category="Behavior")
	bool Running = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	float ChaseRadius = 400.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	float PatienceRadius = 300.0f;

	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void TransitionToPatrolAnimation(float DeltaTime);
	void TransitionToIdleAnimation(float DeltaTime);
	void ScrewWithPlayer();

	UFUNCTION()
	void HandlePatienceTimer(float Val);
	UFUNCTION()
	void HandleStopChaseTimer(float Val);
	UFUNCTION()
	void HandleCrashOutTimer(float Val);

	void ResetKaren();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void MoveToNextPatrolPoint();

	void WaitBeforeMove(float DeltaTime);

	int ChooseRandomPatrolPoint();

	void FollowPlayer();

	UFUNCTION()
	void OnMoveCompletedHandler(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	UFUNCTION(BlueprintCallable)
	ENPCState GetCurrentMoveState(){ return CurrentMoveState;}
	
	UFUNCTION(BlueprintCallable)
	bool GetKarenedOut() const { return KarenedOut;}

private:
	int LastPatrolPoint = 0;
	
	bool IsPowerWalking = false;
	bool ShouldWaitNextMove = false;
	bool PrevMovingToLocation = false;
	bool MovingToLocation = false;

	bool KarenedOut = false;

	float Timer = 0;
	float WaitTimer = 0;
	
	AAIController* AiController = nullptr;
	ENPCState CurrentMoveState = ENPCState::Idle;
	AGroceryRunCharacter* PlayerCharacter;

	FTimeline PatienceTimeline;
	FTimeline StopChaseTimeline;
	FTimeline CrashOutTimeline;

};


