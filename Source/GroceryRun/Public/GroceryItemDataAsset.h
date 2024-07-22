// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GR_Enums.h"
#include "GroceryItemDataAsset.generated.h"

UCLASS(BlueprintType)
class GROCERYRUN_API UGroceryItemDataAsset : public UDataAsset{
	
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grocery")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAisleType Aisle;
	
};


