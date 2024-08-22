// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GRStateDataAsset.h"
#include "Engine/DataAsset.h"
#include "GRTransitionDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class GROCERYRUN_API UGRTransitionDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	FName GetTransitionName() const {return TransitionName;}
	TArray<FName> GetValidationFunctions() const { return ValidationFunctions; }
	UGRStateDataAsset* GetStateTransitioningTo() const {return StateTransitioningTo;}
	FOnContentChanged OnContentChanged;
protected:
	UPROPERTY(EditAnywhere)
	FName TransitionName;
	UPROPERTY(EditAnywhere)
	UGRStateDataAsset* StateTransitioningTo;
	UPROPERTY(EditAnywhere)
	TArray<FName> ValidationFunctions;
};
