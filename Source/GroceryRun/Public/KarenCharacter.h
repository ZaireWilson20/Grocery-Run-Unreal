// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GRBaseCharacter.h"
#include "GRStateDataAsset.h"
#include "WayPoint.h"
#include "GR_Enums.h"
#include "Navigation/PathFollowingComponent.h"
#include "GroceryRun/GroceryRunCharacter.h"
#include "KarenCharacter.generated.h"


class IGRState;
class UGRStateDataAsset;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPushedEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGetUpEvent);


class UGrKarenState;
class UGrKarenIdleState;
class UGrKarenChaseState;
class UGrKarenFallenState;

class UKarenStateMachine;

UCLASS()
class GROCERYRUN_API AKarenCharacter : public AGRBaseCharacter
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
	float TimeToTriggerGetUp = 3.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Timing")
	float TimeToTriggerExitChase = 3.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Timing")
	float TimeToTriggerCrashOut = 3.0f;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPushedEvent OnPushedEvent;
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPushedEvent OnGetUpEvent;


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
	UFUNCTION()
	void HandleDownedTimer(float Val);

	

	UFUNCTION()
	static bool IdleToChase(const AKarenCharacter* KarenCharacter);


	void ResetKaren();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable)
	void MoveToNextPatrolPoint();

	void WaitBeforeMove(float DeltaTime);

	int ChooseRandomPatrolPoint();

	void FollowPlayer();

	UFUNCTION()
	void OnMoveCompletedHandler(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	UFUNCTION()
	void OnPushedByPlayer();

	UFUNCTION(BlueprintCallable)
	ENPCState GetCurrentMoveState(){ return CurrentMoveState;}
	
	UFUNCTION(BlueprintCallable)
	bool GetKarenedOut() const { return KarenedOut;}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="State Machine")
	UKarenStateMachine* StateMachine;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IdleToChaseDebug = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool ChaseToIdleDebug = false;

	

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool TransitionTriggerFirst;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool TransitionTriggerSecond;
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
	FTimeline DownedTimeLine;


	//Delegate for binding UFunction
	FScriptDelegate MyScriptDelegate;
	


};


