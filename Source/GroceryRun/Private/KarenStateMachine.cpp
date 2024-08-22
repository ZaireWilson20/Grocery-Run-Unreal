// Fill out your copyright notice in the Description page of Project Settings.


#include "KarenStateMachine.h"
#include "Algo/Find.h"
#include "UObject/NoExportTypes.h"
#include "Engine/Engine.h"

UKarenStateMachine::UKarenStateMachine()
{
	StatesObjMap.Empty();
	PreviousStates = States;
	CurrentState = nullptr;
}


void UKarenStateMachine::AddTransition(UGRStateDataAsset* from, UGRStateDataAsset* to, const FValidationDelegate& validationFunction)
{
	UGrKarenState* foundFromState = *StatesObjMap.Find(from);
	
	if(!foundFromState){
		//UE_LOG(LogTemp, Error, TEXT("Trying to add transition to a state that doesn't exist in StateMachine state list"));
		return;
	}

	UGrKarenState* foundSToState = *StatesObjMap.Find(to);
	if(!foundSToState){
		//UE_LOG(LogTemp, Error, TEXT("Trying to add transition to a state that doesn't exist in StateMachine state list"));
		return;
	}
	foundSToState->AddTransition(foundFromState, foundSToState, validationFunction);
	
}

UTransition* UKarenStateMachine::AddTransition(UGRStateDataAsset* from, UGRStateDataAsset* to)
{
	UGrKarenState* foundFromState = *StatesObjMap.Find(from);
	
	if(!foundFromState){
		//UE_LOG(LogTemp, Error, TEXT("Trying to add transition to a state that doesn't exist in StateMachine state list"));
		return nullptr;
	}

	UGrKarenState* foundSToState = *StatesObjMap.Find(to);
	if(!foundSToState){
		//UE_LOG(LogTemp, Error, TEXT("Trying to add transition to a state that doesn't exist in StateMachine state list"));
		return nullptr;
	}
	
	return foundSToState->AddTransition(foundFromState, foundSToState);
}

void UKarenStateMachine::BindStateUpdateDelegate(UGRStateDataAsset* state, const TScriptDelegate<>& updateFunc)
{
	/*UGrKarenState** foundState = Algo::FindByPredicate(States, [state](const UGrKarenState* stateA)
	{
		return stateA->StaticClass() == state->StaticClass();
	});*/

	UGrKarenState* foundState = *StatesObjMap.Find(state);
	if(!foundState){
		//UE_LOG(LogTemp, Error, TEXT("Trying to add transition to a state that doesn't exist in StateMachine state list"));
		return;
	}
	foundState->GetOnEnterDelegate().Add(updateFunc);
}

void UKarenStateMachine::BindStateEnterDelegate(UGRStateDataAsset* state, const TScriptDelegate<>& enterFunc)
{
	/*UGrKarenState** foundState = Algo::FindByPredicate(States, [state](const UGrKarenState* stateA)
	{
		return stateA->StaticClass() == state->StaticClass();
	});*/

	UGrKarenState* foundState = *StatesObjMap.Find(state);
	if(!foundState){
		//UE_LOG(LogTemp, Error, TEXT("Trying to add transition to a state that doesn't exist in StateMachine state list"));
		return;
	}
	foundState->GetOnEnterDelegate().Add(enterFunc);
}

void UKarenStateMachine::BindStateExitDelegate(UGRStateDataAsset* state, const TScriptDelegate<>& exitFunc)
{
	/*UGrKarenState** foundState = Algo::FindByPredicate(States, [state](const UGrKarenState* stateA)
	{
		return stateA->StaticClass() == state->StaticClass();
	});*/
	UGrKarenState* foundState = *StatesObjMap.Find(state);
	if(!foundState){
		//UE_LOG(LogTemp, Error, TEXT("Trying to add transition to a state that doesn't exist in StateMachine state list"));
		return;
	}
	foundState->GetOnEnterDelegate().Add(exitFunc);
}


void UKarenStateMachine::CheckForTransition()
{
	if(!CurrentState)
	{
		return;
	}
	bool changed = false;
	UGrKarenState* potentialNextState = Cast<UGrKarenState>(CurrentState->CheckTransitionValidations(&changed));


	if(changed && potentialNextState){
		CurrentState->Exit();
		CurrentState = potentialNextState;
		CurrentState->Enter();

	} else
	{
		//UE_LOG(LogTemp, Error, TEXT("Current State %s, chagned %s, potential state null %s"), *CurrentState->GetStateName().ToString(), changed ? TEXT("True") : TEXT("False"), potentialNextState ? TEXT("True") : TEXT("False"));

	}
	
}

void UKarenStateMachine::TickFSM(float DeltaTime)
{
	if(!CurrentState)
	{
		return;
	}
	CurrentState->Update(DeltaTime);
	OnTransitionValidationCheck.Broadcast();
	CheckForTransition();
}

void UKarenStateMachine::AddStateToObjMap(UGRStateDataAsset* state)
{
	States.Add(state);
	if(state && !StatesObjMap.Contains(state))
	{
		UGrKarenState* karenStateObj = NewObject<UGrKarenState>();
		karenStateObj->Initialize(state, this);
		StatesObjMap[state] = karenStateObj;
	}
}

void UKarenStateMachine::UpdateStateObjMap()
{
	for(UGRStateDataAsset* data : States)
	{
		if(!data)
		{
			continue;
		}
		if(!StatesObjMap.Contains(data))
		{
			UGrKarenState* karenStateObj = NewObject<UGrKarenState>(this, data->GetStateName());
			karenStateObj->Initialize(data, this);
			//karenStateObj->RegisterComponent();
			//GetOwner()->AddInstanceComponent(karenStateObj);
			StatesObjMap.Add(data, karenStateObj);
		}
	}
	for(auto& elem : StatesObjMap)
	{
		elem.Value->LinkTransitions();
	}
	PreviousStates = States;
}

bool UKarenStateMachine::ShouldValidateTransition(UTransitionData* transitionData)
{
	if(!transitionData) return false;
	if(!StatesObjMap.Contains(transitionData->From)) {UE_LOG(LogTemp, Error, TEXT("State Object Mate Does Not Contain Transition From State")); return false;}
	UGrKarenState* state = StatesObjMap[transitionData->From];
	if(!state) {UE_LOG(LogTemp, Error, TEXT("State is nully")); return false;}
	if(state == CurrentState)
	{
		return true;
	}
	return false;
}

void UKarenStateMachine::BeginPlay()
{
	Super::BeginPlay();
	//RefreshStateDataAssetMap();
	UpdateStateObjMap();
	if(InitialState)
	{
		if(StatesObjMap.Contains(InitialState))
		{
			CurrentState = StatesObjMap[InitialState];
			CurrentState->Enter();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Initial State Has Not Been Created"));
		}
	}
}

void UKarenStateMachine::InitializeOnPlay()
{
	for(auto& elem : StatesObjMap)
	{
		elem.Value->InitializeOnPlay(this);
	}
}

void UKarenStateMachine::ValidateTransition(UTransitionData* transitionData, bool validate)
{
	if(transitionData == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Trying To Validate Transition Null Transition Data Asset"));
		return;
	}
	if(StatesObjMap.Contains(transitionData->From) && StatesObjMap.Contains(transitionData->To))
	{
		StatesObjMap[transitionData->From]->ValidateTransition(StatesObjMap[transitionData->To], validate);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Trying To Validate Transition That Contains a State That Doesn't Exist on the State Machine"));
	}
}

#if WITH_EDITOR

void UKarenStateMachine::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if(PropName == GET_MEMBER_NAME_CHECKED(UKarenStateMachine, States))
	{
		//UpdateStateObjMap();
	}
}

void UKarenStateMachine::PostEditUndo()
{
	Super::PostEditUndo();
	//pdateStateObjMap();

}

void UKarenStateMachine::RefreshStateDataAssetMap()
{
	for(auto& elem : StatesObjMap)
	{
		elem.Value->Reset();
		elem.Value->DataAssetUpdate();
	}
}


#endif
