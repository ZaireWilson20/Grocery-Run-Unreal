// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "GRBaseState.h"
#include "GRState.h"
#include "KarenCharacter.h"
#include "Transition.h"
#include "GRKarenState.generated.h"


/**
 * 
 */
UCLASS(BlueprintType)
class GROCERYRUN_API UGrKarenState : public UGRBaseState
{
	
	GENERATED_BODY()
public:

	UGrKarenState();
	virtual ~UGrKarenState() override;
	virtual void Enter() override;
	virtual void Exit() override;
	virtual void Update(float DeltaTime) override;
	virtual UObject* GetUObjectPtr() override { return this; }
	virtual bool CheckInternalCondition() const override;
	virtual void Reset() override;
	

	bool operator==(const UGrKarenState& rhd) const { return this->State == rhd.State; }

	bool operator!=(const UGrKarenState& rhd) const { return this->State != rhd.State; }

	
	ENPCState State;

private:
	UPROPERTY()
	AKarenCharacter* KarenCharacter;
};
