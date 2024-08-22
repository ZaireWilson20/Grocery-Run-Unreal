#pragma once
#include "CoreMinimal.h"


UENUM(BlueprintType)
enum class EAisleType : uint8 {
	Default UMETA(DisplayName = "Default"),
	Breakfast UMETA(DisplayName ="Breakfast"),
	Dairy UMETA(DisplayName = "Dairy"),
	Drinks UMETA(DisplayName = "Drinks")
	
};

UENUM(BlueprintType)
enum class ENPCState : uint8{
	Idle UMETA(DisplayName = "Idle"),
	Patrol UMETA(DisplayName = "Patrol"),
	Chase UMETA(DisplayName = "Chase"),
	Attack UMETA(DisplayName = "Attack"),
	Wait UMETA(DisplayName = "Wait"),
	Downed UMETA(DisplayName = "Downed")
};

UENUM(BlueprintType)
enum class EPickupType : uint8{
	Speed UMETA(DisplayName = "Speed"),
	Health UMETA(DisplayName = "Health"),
};



class GR_Enums {

	
};
