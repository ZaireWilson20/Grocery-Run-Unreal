// Copyright Epic Games, Inc. All Rights Reserved.

#include "GroceryRunGameMode.h"
#include "GroceryRunCharacter.h"
#include "UObject/ConstructorHelpers.h"

AGroceryRunGameMode::AGroceryRunGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
