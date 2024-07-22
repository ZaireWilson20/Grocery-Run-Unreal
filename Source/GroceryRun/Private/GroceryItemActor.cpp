// Fill out your copyright notice in the Description page of Project Settings.


#include "GroceryItemActor.h"

// Sets default values
AGroceryItemActor::AGroceryItemActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGroceryItemActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGroceryItemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

