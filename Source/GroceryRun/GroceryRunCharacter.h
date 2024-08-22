// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GroceryItemDataAsset.h"
#include "GR_Enums.h"
#include "Components/BoxComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/TimelineComponent.h"
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
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRunStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRunEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPickupStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPickupEnded);



struct FGroceryInventoryItem{
	FString Name;
	EAisleType Aisle;
	
	// Constructor to initialize from a Data Asset
	FGroceryInventoryItem()
		: Name(""), Aisle(EAisleType::Breakfast){}

	FGroceryInventoryItem(UGroceryItemDataAsset* DataAsset){
		if (DataAsset){
			Name = DataAsset->Name;
			Aisle = DataAsset->Aisle;
		}
	}
};

UCLASS(config=Game)
class AGroceryRunCharacter : public ACharacter{
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

	/** Crouch Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* CrouchAction;

	/** Lock Aim Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* LockAimAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Push Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* PushAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;
	float InitialYaw;

public:
	AGroceryRunCharacter();

	// Event Dispatcher
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHoverEnterEvent OnHoverEnter;
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHoverEnterEvent OnHoverExit;
	UPROPERTY(BlueprintAssignable, Category= "Events")
	FOnRunStarted RunningStarted;
	UPROPERTY(BlueprintAssignable, Category= "Events")
	FOnRunEnded RunningEnded;
	UPROPERTY(BlueprintAssignable, Category= "Events")
	FOnPickupStarted PickingUpStarted;
	UPROPERTY(BlueprintAssignable, Category= "Events")
	FOnPickupEnded PickingUpEnded;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
	UPostProcessComponent* PostProcessComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Materials")
	UMaterialInstanceDynamic* DynamicMat; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Timing")
	float MaxGroceryPickUpTimer = 5.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Timing")
	float MaxSpeedBoostTime = 10.0;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Timing")
	float FOV_Transition_time = 1.5;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Timing")
	UCurveFloat* CrouchCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CameraBehaviour")
	float NormalFOV = 90;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CameraBehaviour")
	float PickupFOV = 70;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CameraBehaviour")
	float MaxVignetteStrength = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CameraBehaviour")
	float MaxVignetteZoom = .5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CameraBehaviour")
	float BaseCapHeight = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CameraBehaviour")
	float CrouchHeight = 50;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PlayerAttribute")
	int MaxHealth = 3;
	UPROPERTY(BlueprintReadOnly);
	bool MovementLocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AuxReferences")
	UBoxComponent* PushCollider = nullptr;
	
	
	// Function to trigger the event
	UFUNCTION(BlueprintCallable, Category = "Events")
	void TriggerOnHoverEnter();
	UFUNCTION(BlueprintCallable, Category = "Events")
	void TriggerOnHoverExit();
	UFUNCTION(BlueprintCallable, Category="Events")
	void BindDelegates();

	UFUNCTION(BlueprintCallable)
	void LosePatience();
	UFUNCTION(BlueprintCallable)
	void GainPatience();

	UFUNCTION(BlueprintCallable)
	bool HasPlayerLostPatience() const {
		return Health == 0;
	}

	
#pragma region GETTERS_SETTERS

	UFUNCTION(BlueprintCallable)
	float GetHealth() const{
		return Health;
	}
	
	UFUNCTION(BlueprintCallable)
	float GetSpeedBoostTimer() const{
		return SpeedBoostTimer;
	}
	
	UFUNCTION(BlueprintCallable)
	float GetGroceryPickupTimer() const{
		return GroceryPickUpTimer;
	}
#pragma endregion

protected:
	virtual void BeginPlay() override;

	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	void HandleFOVTransition(float DeltaTime);
	virtual void Tick(float DeltaTime) override;

	virtual void Jump() override;
	
	/** Called for movement input */
	void Move(const FInputActionValue& Value);
	void Push(const FInputActionValue& Value);
	void Push_Finished(const FInputActionValue& Value);
	
	void LockOnGroceryStart(const FInputActionValue& Value);
	void LockOnGroceryStartManual();
	
	void LockOnGroceryEnd(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	void CharacterCrouch(const FInputActionValue& Value);

	void CharacterUnCrouch(const FInputActionValue& Value);

	void PickUpModifier(EPickupType Pickup, float Val);

	void ResetSpeed();

	void HandleGroceryFocusTiming(float DeltaTime);
	
	void HandleSpeedPickupTiming(float DeltaTime);

	UFUNCTION()
	void HandleCrouchProgress(float Val);

	UFUNCTION(BlueprintCallable)
	void OnSpeedPickup(float Val);
	
	UFUNCTION(BlueprintCallable)
	void OnHealthPickup(float Val);

	UFUNCTION()
	void TriggerIfKarenPushed(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
	

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
	
	bool Crouched = false;
	bool Crouching = false;
	bool HasSpeedBoost = false;
	bool IsSetup = false;
	bool LookingAtGroceryItem = false;
	bool lookingAtKaren = false;
	bool ToPickupFOV = false;
	bool TriggerFOVTransition = false;
	bool UnCrouching = false;

	FRotator InitialRotation;

	float BaseMoveSpeed = 0;
	float BaseMoveSpeedCrouched = 0;
	float FOV_TransitionTimer = 0;
	float GroceryPickUpTimer = 0;
	float NoiseAmplitude;
	float NoiseSpeed;
	float NoiseTimeX = 0;
	float NoiseTimeY = 0;
	float PrevControllerPitch;
	float PrevControllerYaw;
	float SmoothPitch;
	float SmoothYaw;
	float SpeedBoostTimer = 0;
	float SwayAmplitude;
	float SwayFrequency;
	float SwayTime;
	
	int Health = 0;
	
	TArray<FGroceryInventoryItem> BagInventory;

	UPROPERTY()
	UGroceryItemDataAsset* ItemInFocus = nullptr;
	UPROPERTY()
	AActor* ActorInFocus = nullptr;

	FTimeline CrouchTimeline;
	FScriptDelegate PushColliderOverlapDelegate;


	FVector2D GetCameraViewportCenter() const;
	
};

