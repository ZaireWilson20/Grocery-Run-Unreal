// Fill out your copyright notice in the Description page of Project Settings.


#include "GRKarenState.h"

UGrKarenState::UGrKarenState()
{
}

UGrKarenState::~UGrKarenState()
{
}

void UGrKarenState::Enter()
{
	//IGRState::Enter();
	UE_LOG(LogTemp, Warning, TEXT("Entering Current State: %s"), *StateName.ToString());
	OnEnterDelegate.Broadcast();
}

void UGrKarenState::Exit()
{
	//IGRState::Exit();
	UE_LOG(LogTemp, Warning, TEXT("Exiting Current State: %s"), *StateName.ToString());
	OnExitDelegate.Broadcast();
}

void UGrKarenState::Update(float DeltaTime)
{
	//IGRState::Update();
	OnUpdateDelegate.Broadcast(DeltaTime);
}



bool UGrKarenState::CheckInternalCondition() const
{
	return InternalConditionTriggered;
}

void UGrKarenState::Reset()
{
	Super::Reset();
}






