// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NPCCharacter.generated.h"

UCLASS()
class GROCERYRUN_API ANPCCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ANPCCharacter();


protected:


	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:

	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Navigation")
	TArray<int> PatrolPoints;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Behavior")
	int PatrolSize;
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


};
