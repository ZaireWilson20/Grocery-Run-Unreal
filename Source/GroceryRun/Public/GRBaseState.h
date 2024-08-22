// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GRBaseCharacter.h"
#include "GRState.h"
#include "GRStateDataAsset.h"
#include "UObject/Object.h"
#include "Transition.h"
#include "GRBaseState.generated.h"


class UKarenStateMachine;
DECLARE_DELEGATE(FStateDelegateSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStateDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStateUpdateDelegate, double, DeltaTime);
struct FTransitionStruct;

/*
USTRUCT(BlueprintType)
struct FStateTransition
{
	GENERATED_BODY()

	FStateTransition(UTransition* transition)
	{
		From = transition->GetFromState();
		To = transition->GetToState();
		for(FValidationDelegate val : transition->GetValidationDelegates())
		{
			ValidationDelegates.Add(val);
		}
	}
	UPROPERTY(EditAnywhere)
	UGRBaseState* From;
	UPROPERTY(EditAnywhere)
	UGRBaseState* To;
	UPROPERTY(EditAnywhere)
	TArray<FValidationDelegate> ValidationDelegates;

	bool operator==(const FStateTransition& Other) const
	{
		for(FValidationDelegate val : ValidationDelegates)
		{
			for(FValidationDelegate valB : Other.ValidationDelegates)
			{
				if(!UTransition::CompareValidationDelegates(val, valB))
				{
					return false;
				}
			}
		}
		return From == Other.From && To == Other.To;
	}
};
*/

/*FORCEINLINE uint32 GetTypeHash(const FStateTransition& MyStruct)
{
	return HashCombine(GetTypeHash(MyStruct.From), GetTypeHash(MyStruct.To));
}*/
/**
 * 
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GROCERYRUN_API UGRBaseState : public UObject, public IGRState
{
	GENERATED_BODY()
public:
	virtual void Initialize(UGRStateDataAsset* dataAsset, UKarenStateMachine* stateMachine);
	virtual void InitializeOnPlay(UKarenStateMachine* fsm);

	virtual void Enter() override;
	virtual void Exit() override;
	virtual void Update(float DeltaTime) override;
	virtual UObject* GetUObjectPtr() override { return this; }
	virtual bool CheckInternalCondition() const override;
	virtual void Reset();
	void DataAssetUpdate();
	UGRStateDataAsset* GetDataAssetParent () const { return DataAssetParent;}

	UFUNCTION(BlueprintImplementableEvent, Category = "StateEvents")
	void OnEnterEvent();
	UFUNCTION(BlueprintImplementableEvent, Category = "StateEvents")
	void OnExitEvent();
	UFUNCTION(BlueprintImplementableEvent, Category = "StateEvents")
	void OnUpdateEvent(double DeltaTime);

	
	UTransition* AddTransition(UGRBaseState* from, UGRBaseState* to, const FValidationDelegate& ValidationDelegate);
	UTransition* AddTransition(UGRBaseState* from, UGRBaseState* to);
	void SetTransitionValidationDelegate(UTransition* transition, const FValidationDelegate& ValidationDelegate);
	FName GetStateName() const {return StateName;}
	UKarenStateMachine* GetStateMachine() const {return StateMachine;}
	FStateDelegate GetOnEnterDelegate();
	FStateDelegate GetOnExitDelegate();
	FStateUpdateDelegate GetOnUpdateDelegate();
	virtual UGRBaseState* CheckTransitionValidations(bool* changed);
	void ValidateTransition(const UGRBaseState* ToState, bool validate);
	void LinkTransitions();
	bool HasTransition(const UTransitionData* transitionData);

#if WITH_EDITOR
	// Called when a property is edited in the Unreal Editor
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditUndo() override;
#endif


protected:
	bool InternalConditionTriggered = false;
	UPROPERTY(EditAnywhere, BlueprintAssignable)
	FStateDelegate OnEnterDelegate;
	UPROPERTY(EditAnywhere, BlueprintAssignable)
	FStateDelegate OnExitDelegate;
	UPROPERTY(EditAnywhere, BlueprintAssignable)
	FStateUpdateDelegate OnUpdateDelegate;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TransitionContainer")
	TSet<UTransition*> Transitions;
	UPROPERTY()
	TSet<UTransition*> PrevTransitions;
	UPROPERTY(EditAnywhere, Category="TransitionContainer")
	UKarenStateMachine* StateMachine;
	FName StateName;
	UPROPERTY(EditAnywhere, Category="TransitionContainer")
	TMap<UTransitionData*, UTransition*> TransitionObjMap;
	UPROPERTY(EditAnywhere, Category="TransitionContainer")
	UGRStateDataAsset* DataAssetParent = nullptr;


private:
   

};
