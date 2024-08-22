// Fill out your copyright notice in the Description page of Project Settings.


#include "Transition.h"

#include "GRBaseState.h"
#include "KarenStateMachine.h"


void UTransition::Initialize(FName state)
{
	FromState = nullptr;
	ToState = nullptr;
	FString str;
	str.Append("_Transition");

	TransitionName = state;
	TransitionName.AppendString(str);
}

void UTransition::Initialize(UGRBaseState* from, UGRBaseState* to)
{
	FromState = from;
	ToState = to;
}

void UTransition::Initialize(const UGRTransitionDataAsset* transitionAsset, UGRBaseState* from)
{
	/*DataAssetParent = transitionAsset;
	FromState = from;
	TransitionName = transitionAsset->GetTransitionName();
	UKarenStateMachine* stateMachine = from->GetStateMachine();
	if(!stateMachine){
		UE_LOG(LogTemp, Warning, TEXT("State Machine not attached to state on transition intialize"));
		return;
	}
	StateMachine = stateMachine;
	UGRBaseState* toState = stateMachine->GetStateObject(transitionAsset->GetStateTransitioningTo());
	if(!toState){
		UE_LOG(LogTemp, Warning, TEXT("State transitioning to doesn't exist in State Machine on transition intialize"));
		return;
	}
	ToState = stateMachine->GetStateObject(transitionAsset->GetStateTransitioningTo());*/
}

void UTransition::Initialize(UTransitionData* transStruct, UGRBaseState* from, UGRBaseState* to)
{
	TransitionDataAsset = transStruct;
	FromState = from;
	TransitionName = transStruct->TransitionName;
	UKarenStateMachine* stateMachine = from->GetStateMachine();
	if(!stateMachine){
		UE_LOG(LogTemp, Warning, TEXT("State Machine not attached to state on transition intialize"));
		return;
	}
	StateMachine = stateMachine;
	if(!to){
		UE_LOG(LogTemp, Warning, TEXT("State transitioning to doesn't exist in State Machine on transition intialize"));
		return;
	}
	ToState = to;
}

void UTransition::InitializeOnPlay(UKarenStateMachine* fsm)
{
	StateMachine = fsm;
}


void UTransition::AddValidationDelegate(const FValidationDelegate& validationDelegate)
{
	if(!validationDelegate.IsBound())
	{
		UE_LOG(LogTemp, Warning, TEXT("Trying to add unbound delegate to Transition Validation"));
		return;
	}
	ValidationDelegates.Add(validationDelegate);
}


UGRBaseState* UTransition::GetToState() const
{
	return ToState;
}

UGRBaseState* UTransition::GetFromState() const
{
	return FromState;
}

TArray<FValidationDelegate> UTransition::GetValidationDelegates()
{
	return ValidationDelegates;
}

bool UTransition::InvokeTransitionValidation()
{


	/*for(const FValidationDelegate& del : ValidationDelegates)
	{
		if(del.IsBound())
		{
			if(!del.Execute(FromState))
			{
				return false;
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Unbound delegate found in ValidationDelegates"));
		}

	}*/

	return CanTransition;

}

bool UTransition::CompareValidationDelegates(const FValidationDelegate& delA, const FValidationDelegate& delB)
{
	// Check if both delegates are bound
	if (delA.IsBound() && delB.IsBound())
	{
		// Compare the object they are bound to (if any)
		if (delA.GetUObject() == delB.GetUObject())
		{
			// Compare the function names they are bound to
			if (delA.GetFunctionName() == delB.GetFunctionName())
			{
				return true;
				//UE_LOG(LogTemp, Warning, TEXT("The delegates are bound to the same function!"));
			}
			return false;
			//UE_LOG(LogTemp, Warning, TEXT("The delegates are bound to different functions."));
		}
		return false;
		//UE_LOG(LogTemp, Warning, TEXT("The delegates are bound to different objects."));
	}


	if (!delA.IsBound() && !delB.IsBound())
	{
		UE_LOG(LogTemp, Warning, TEXT("Both delegates are unbound and thus considered equal."));
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("One delegate is bound and the other is not."));
	return false;
}


