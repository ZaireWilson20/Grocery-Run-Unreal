// Fill out your copyright notice in the Description page of Project Settings.


#include "GRTransitionDataAsset.h"

void UGRTransitionDataAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	OnContentChanged.Broadcast();
}
