// Fill out your copyright notice in the Description page of Project Settings.


#include "GRKarenIdleState.h"

UGrKarenIdleState::UGrKarenIdleState()
{
}

UGrKarenIdleState::~UGrKarenIdleState()
{
}

void UGrKarenIdleState::Enter()
{
	Super::Enter();
}

void UGrKarenIdleState::Update(float DeltaTime)
{
	Super::Update(DeltaTime);
	InternalTimer.TickTimeline(DeltaTime);
}

void UGrKarenIdleState::Exit()
{
	Super::Exit();
}


void UGrKarenIdleState::HandleInternalTimerFinished()
{
	InternalTimerFinished = true;
}
