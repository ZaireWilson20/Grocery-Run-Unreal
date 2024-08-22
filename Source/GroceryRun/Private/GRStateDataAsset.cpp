// Fill out your copyright notice in the Description page of Project Settings.


#include "GRStateDataAsset.h"

#include "GRTransitionDataAsset.h"


void  UGRStateDataAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	FName PropName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if(PropName == GET_MEMBER_NAME_CHECKED(UGRStateDataAsset, Transitions))
	{
		TSet<UTransitionData*> ToRemove;
		for(UTransitionData* u : Transitions)
		{

			if(!u)
			{
				continue;
			}
			if(u->From != this)
			{
				UE_LOG(LogTemp, Error, TEXT("State Asset must match the From value in Transition Asset"));
				ToRemove.Add(u);
			}
		}
		Transitions = Transitions.Difference(ToRemove);
	}
	OnContentChanged.Broadcast();
}
