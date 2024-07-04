// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "GroceryRunCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHoverEnterEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHoverExitEvent);

UENUM(BlueprintType)
enum class EPickupType : uint8{
	Speed UMETA(DisplayName = "Speed"),
	Health UMETA(DisplayName = "Health"),
};

UCLASS(config=Game)
class AGroceryRunCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Mesh, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh1P;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* CrouchAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* MoveAction;
	
public:
	AGroceryRunCharacter();

	// Event Dispatcher
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHoverEnterEvent OnHoverEnter;
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHoverEnterEvent OnHoverExit;

	// Function to trigger the event
	UFUNCTION(BlueprintCallable, Category = "Events")
	void TriggerOnHoverEnter();
	UFUNCTION(BlueprintCallable, Category = "Events")
	void TriggerOnHoverExit();

protected:
	virtual void BeginPlay();

	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;
	
	virtual void Tick(float DeltaTime) override;


public:
		
	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxSpeedBoostTime = 10.0;
protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	void CharacterCrouch(const FInputActionValue& Value);

	void CharacterUnCrouch(const FInputActionValue& Value);

	void PickUpModifier(EPickupType Pickup, float Val);

	UFUNCTION(BlueprintCallable)
	void OnSpeedPickup(float Val);

	void ResetSpeed();
	
	UFUNCTION(BlueprintCallable)
	void OnHealthPickup(float Val);



protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

public:
	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	void ShootCameraLineTrace();

private:
	bool lookingAtKaren = false;
	bool HasSpeedBoost = false;
	
	float SpeedBoostTimer = 0;
	float BaseMoveSpeed = 0;
	float BaseMoveSpeedCrouched = 0;
	

};

