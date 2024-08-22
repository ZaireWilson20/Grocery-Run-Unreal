// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GRKarenState.h"
#include "GRKarenIdleState.generated.h"
/**
 * 
 */
UCLASS(Blueprintable)
class GROCERYRUN_API UGrKarenIdleState : public UGrKarenState
{
	GENERATED_BODY()
public:
	UGrKarenIdleState();
	~UGrKarenIdleState();

	virtual void Enter() override;
	virtual void Update(float DeltaTime) override;
	virtual void Exit() override;
	//virtual void Initialize(AKarenCharacter* karen) override;

	UPROPERTY()
	UCurveFloat* LinearMinuteTimeCurve;

	bool InternalTimerFinished = false;

private:
	void HandleInternalTimerFinished();
	
	FTimeline InternalTimer;

};
