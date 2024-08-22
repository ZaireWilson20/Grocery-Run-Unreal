// Fill out your copyright notice in the Description page of Project Settings.


#include "GRBaseState.h"
#include "KarenStateMachine.h"
#include "GRStateDataAsset.h"



void UGRBaseState::Initialize(UGRStateDataAsset* dataAsset, UKarenStateMachine* stateMachine)
{
	StateName = dataAsset->GetStateName();
	StateMachine = stateMachine;
	Transitions.Empty();
	TransitionObjMap.Empty();
	for(UTransitionData* data : dataAsset->GetTransitions())
	{

		FString str1 = GetStateName().ToString() + TEXT("_Transition");
		FName str(*str1);
		str.AppendString(str1);
		UTransition* newTransition = NewObject<UTransition>(this, str);
		UGRBaseState* tempToState = stateMachine->GetStateObject(data->To);

		newTransition->Initialize(data, this, tempToState);
		Transitions.Add(newTransition);
		TransitionObjMap.Add(data, newTransition);

	}
	DataAssetParent = dataAsset;
}

void UGRBaseState::InitializeOnPlay(UKarenStateMachine* fsm)
{
	StateMachine = fsm;
	for(UTransition* elem : Transitions)
	{
		elem->InitializeOnPlay(StateMachine);
	}
}

void UGRBaseState::Enter()
{
	//IGRState::Enter();
	if(OnEnterDelegate.IsBound())
	{
		OnEnterDelegate.Broadcast();
	}

	OnEnterEvent();
}

void UGRBaseState::Exit()
{
	//IGRState::Exit();
	if(OnExitDelegate.IsBound())
	{
		OnExitDelegate.Broadcast();
	}
	OnExitEvent();
}

void UGRBaseState::Update(float DeltaTime)
{
	//IGRState::Update();
	if(OnUpdateDelegate.IsBound())
	{
		OnUpdateDelegate.Broadcast(DeltaTime);
	}
	OnUpdateEvent(DeltaTime);
}


bool UGRBaseState::CheckInternalCondition() const
{
	return InternalConditionTriggered;
}

void UGRBaseState::Reset()
{
	Transitions.Empty();
	TransitionObjMap.Empty();
	PrevTransitions.Empty();
	
}

void UGRBaseState::DataAssetUpdate()
{
	if(!DataAssetParent)
	{
		UE_LOG(LogTemp, Warning, TEXT("State Has No Parent Data Asset"));
		return;
	}
	if(!StateMachine)
	{
		UE_LOG(LogTemp, Warning, TEXT("No State Machine Reference"));
		return;
	}
	StateName = DataAssetParent->GetStateName();
	for(UTransitionData* transData : DataAssetParent->GetTransitions())
	{
		if(!TransitionObjMap.Contains(transData))
		{
			UGRBaseState* tempTo = StateMachine->GetStateObject(transData->To);
			if(!tempTo)
			{
				UE_LOG(LogTemp, Warning, TEXT("State Transitioning To Doesn't Exist at DataAssetUpdate()"));
				continue;
			}
			FString str1 = GetStateName().ToString() + TEXT("_Transition");
			FName str(*str1);
			str.AppendString(str1);
			UTransition* newTransition = NewObject<UTransition>(this, str);

			newTransition->Initialize(transData, this, tempTo);
			TransitionObjMap.Add(transData, newTransition);
		}
		else
		{
			UGRBaseState* tempTo = StateMachine->GetStateObject(transData->To);
			if(!tempTo)
			{
				UE_LOG(LogTemp, Warning, TEXT("State Transitioning To Doesn't Exist at DataAssetUpdate()"));
				continue;
			}
			TransitionObjMap[transData]->Initialize(transData,this, tempTo);
		}
	}
}


UTransition* UGRBaseState::AddTransition(UGRBaseState* from, UGRBaseState* to, const FValidationDelegate& ValidationDelegate)
{
	/*if(!from || !to){
		UE_LOG(LogTemp, Warning, TEXT("State that you are trying to add transition to don't exist"));
		return nullptr;
	}
	UTransition* newTransition = NewObject<UTransition>();
	newTransition->Initialize(from, to);
	newTransition->AddValidationDelegate(ValidationDelegate);
	Transitions.Add(newTransition);*/
	return nullptr;
}

UTransition* UGRBaseState::AddTransition(UGRBaseState* from, UGRBaseState* to)
{
	/*if(!from || !to){
		UE_LOG(LogTemp, Warning, TEXT("State that you are trying to add transition to don't exist"));
		return nullptr;
	}
	UTransition* newTransition = NewObject<UTransition>();
	newTransition->Initialize(from, to);
	Transitions.Add(newTransition);*/
	return nullptr;
}

void UGRBaseState::SetTransitionValidationDelegate(UTransition* transition, const FValidationDelegate& ValidationDelegate)
{
	UTransition** transitionFound = Transitions.Find(transition);
	if(transitionFound == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Transition Doesn't Exist in Transition List"));
		return;	
	}
	(*transitionFound)->AddValidationDelegate(ValidationDelegate);
}

FStateDelegate UGRBaseState::GetOnEnterDelegate()
{
	return OnEnterDelegate;
}

FStateDelegate UGRBaseState::GetOnExitDelegate()
{
	return OnExitDelegate;
}

FStateUpdateDelegate UGRBaseState::GetOnUpdateDelegate()
{
	return OnUpdateDelegate;
}


/**
 * Finds the first validated transition and updates the Parent StateMachine's changed status. Returns the state pointed to.
 * @param changed reference to StateMachine's changed status
 * @return Karen State
 */
UGRBaseState* UGRBaseState::CheckTransitionValidations(bool* changed)
{
	for(UTransition* elem : Transitions)
	{
		UTransition* currentTransIteration = elem;
		if(currentTransIteration->ReadyToTransition())
		{
			*changed = true;
			UGRBaseState* tempBaseState = currentTransIteration->GetToState();
			if(!tempBaseState)
			{
				UE_LOG(LogTemp, Warning, TEXT("Transition To State Invalid Cast To Karen State"));
				return nullptr;
			}
			return tempBaseState;

		}
	}
	return nullptr;
}

void UGRBaseState::ValidateTransition(const UGRBaseState* ToState, bool validate)
{
	if(ToState == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Trying To Validate Null Pointer State"));
		return;
	}
	
	for(UTransition* transition : Transitions)
	{
		if(transition->GetToState() == ToState)
		{
			transition->SetCanTransition(validate);
			return;
		}
		UE_LOG(LogTemp, Warning, TEXT("Trying To ToState That Doesn't exist"));
	}
}

void UGRBaseState::LinkTransitions()
{
	for(UTransition* trans : Transitions)
	{
		UGRBaseState* to = StateMachine->GetStateObject(trans->GetMappedTransitionDataAsset()->To);
		if(to)
		{
			trans->Initialize(trans->GetMappedTransitionDataAsset(), this, to);
		}
			
	}
}

bool UGRBaseState::HasTransition(const UTransitionData* transitionData)
{
	for(UTransition* transition : Transitions)
	{
		if(transition->GetMappedTransitionDataAsset() == transitionData)
		{
			return true;
		}
	}
	return false;
}

void UGRBaseState::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	FName PropName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if(PropName == GET_MEMBER_NAME_CHECKED(UGRBaseState, Transitions))
	{
	}
}

void UGRBaseState::PostEditUndo()
{
	Super::PostEditUndo();
}


