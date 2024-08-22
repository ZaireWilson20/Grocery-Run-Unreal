// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GRStateDataAsset.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnContentChanged);
class UGRTransitionDataAsset;


UCLASS()
class UTransitionData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName TransitionName;
	UPROPERTY(EditAnywhere)
	UGRStateDataAsset* From;
	UPROPERTY(EditAnywhere)
	UGRStateDataAsset* To;
	UPROPERTY(EditAnywhere)
	TArray<FName> ValidationFunctions;
};


/**
 * 
 */

UCLASS()
class GROCERYRUN_API UGRStateDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	FName GetStateName() const {return StateName;}
	TSet<UTransitionData*> GetTransitions() const {return Transitions;}
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	FOnContentChanged OnContentChanged;
protected:
	UPROPERTY(EditAnywhere)
	FName StateName;
	UPROPERTY(EditAnywhere)
	TSet<UTransitionData*> Transitions; 
};


