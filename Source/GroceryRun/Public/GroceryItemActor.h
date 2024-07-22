// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GroceryItemDataAsset.h"
#include "GameFramework/Actor.h"
#include "GroceryItemActor.generated.h"

UCLASS()
class GROCERYRUN_API AGroceryItemActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGroceryItemActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UGroceryItemDataAsset* ItemData;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
};
