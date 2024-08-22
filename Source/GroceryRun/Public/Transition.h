// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GRTransitionDataAsset.h"
#include "UObject/Object.h"
#include "Transition.generated.h"


class UKarenStateMachine;
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FValidationDelegate, const UGRBaseState*, state);
class UGRBaseState;

/**
 * 
 */

UCLASS(BlueprintType)
class GROCERYRUN_API UTransition : public UObject
{
GENERATED_BODY()
public:
	
	virtual void Initialize(FName state);
	virtual void Initialize(UGRBaseState* from, UGRBaseState* to);
	virtual void Initialize(const UGRTransitionDataAsset* transitionAsset, UGRBaseState* from);
	virtual void Initialize(UTransitionData* transStruct, UGRBaseState* from, UGRBaseState* to);
	virtual void InitializeOnPlay(UKarenStateMachine* fsm);
	UFUNCTION(BlueprintCallable, Category="Validation")
	void AddValidationDelegate(const FValidationDelegate& validationDelegate);
	void SetCanTransition(bool valid){ CanTransition  = valid; }

	UGRBaseState* GetToState() const;
	UGRBaseState* GetFromState() const;
	TArray<FValidationDelegate> GetValidationDelegates();
	bool InvokeTransitionValidation();
	UTransitionData* GetMappedTransitionDataAsset() const { return TransitionDataAsset; }
	bool ReadyToTransition() const { return CanTransition; }
	static bool CompareValidationDelegates(const FValidationDelegate& delA, const FValidationDelegate& delB);
	
	FName TransitionName;
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Validation")
	TArray<FValidationDelegate> ValidationDelegates;
	UPROPERTY(VisibleAnywhere)
	UGRBaseState* FromState;
	UPROPERTY(VisibleAnywhere)
	UGRBaseState* ToState;
	UPROPERTY(BlueprintReadWrite)
	bool BlueprintEventValidation = true;
	UPROPERTY(VisibleAnywhere)
	UTransitionData* TransitionDataAsset;
	UPROPERTY()
	UKarenStateMachine* StateMachine;
	UPROPERTY(VisibleAnywhere)
	bool CanTransition;
private:
	



};

