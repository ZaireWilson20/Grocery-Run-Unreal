// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GRState.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGRState : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class GROCERYRUN_API IGRState
{    
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	
	virtual void Enter() = 0;
	virtual void Update(float DeltaTime) = 0;
	virtual void Exit() = 0;
	virtual bool CheckInternalCondition() const = 0;
	virtual UObject* GetUObjectPtr() = 0;
};
