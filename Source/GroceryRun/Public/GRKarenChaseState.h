// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GRKarenState.h"
#include "GRKarenChaseState.generated.h"
/**
 * 
 */
UCLASS(Blueprintable)
class GROCERYRUN_API UGrKarenChaseState : public UGrKarenState
{

	GENERATED_BODY()
	
public:
	UGrKarenChaseState();
	~UGrKarenChaseState();
};
