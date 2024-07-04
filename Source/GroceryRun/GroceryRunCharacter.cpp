// Copyright Epic Games, Inc. All Rights Reserved.

#include "GroceryRunCharacter.h"
#include "GroceryRunProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "KarenCharacter.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AGroceryRunCharacter

AGroceryRunCharacter::AGroceryRunCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
		
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	//Mesh1P->SetRelativeRotation(FRotator(0.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

}

void AGroceryRunCharacter::Tick(float DeltaTime){
	ShootCameraLineTrace();
	if(HasSpeedBoost){
		if(SpeedBoostTimer >= MaxSpeedBoostTime){
			ResetSpeed();
			HasSpeedBoost = false;
			SpeedBoostTimer = 0;
		} else {
			SpeedBoostTimer += DeltaTime;
		}
	}
}

void AGroceryRunCharacter::TriggerOnHoverEnter(){
	OnHoverEnter.Broadcast();
}

void AGroceryRunCharacter::TriggerOnHoverExit(){
	OnHoverExit.Broadcast();
}

void AGroceryRunCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	BaseMoveSpeed = GetCharacterMovement()->MaxWalkSpeed;
	BaseMoveSpeedCrouched = GetCharacterMovement()->MaxWalkSpeedCrouched;

}

void AGroceryRunCharacter::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp,
	bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit){
	
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	//if(Other->IsA(Pickup))
}

//////////////////////////////////////////////////////////////////////////// Input

void AGroceryRunCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Crouching
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &AGroceryRunCharacter::CharacterCrouch);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AGroceryRunCharacter::CharacterUnCrouch);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGroceryRunCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGroceryRunCharacter::Look);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AGroceryRunCharacter::ShootCameraLineTrace(){
	FVector cameraCenterStart = FirstPersonCameraComponent->GetComponentLocation();
	FVector cameraLookDir = FirstPersonCameraComponent->GetForwardVector();

	FVector end = ((cameraLookDir * 1000.f) + cameraCenterStart);

	FCollisionQueryParams colParams;
	colParams.AddIgnoredActor(this);

	FHitResult hit;
	bool hitObj = GetWorld()->LineTraceSingleByChannel(hit, cameraCenterStart, end, ECC_Visibility, colParams);
	
	if(hitObj && hit.GetActor()->ActorHasTag("Focusable")){
		if(!lookingAtKaren){
			TriggerOnHoverEnter();
			UE_LOG(LogTemp, Warning, TEXT("Karen Spotted On Hover Enter!"));
		}
		lookingAtKaren = true;
	} else {
		if(lookingAtKaren){
			TriggerOnHoverExit();
			UE_LOG(LogTemp, Warning, TEXT("Stopped Looking At Karen!"));

		}
		lookingAtKaren = false;	
	}
	
}


void AGroceryRunCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AGroceryRunCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AGroceryRunCharacter::CharacterCrouch(const FInputActionValue& Value)
{
	if(Controller != nullptr){
		Crouch();
	}
}

void AGroceryRunCharacter::CharacterUnCrouch(const FInputActionValue& Value)
{
	
	if(Controller != nullptr){
		UnCrouch();
	}
}

void AGroceryRunCharacter::PickUpModifier(EPickupType Pickup, float Val)
{
	switch (Pickup)
	{
	case EPickupType::Speed:
		OnSpeedPickup(Val);
		break;
	case EPickupType::Health:
		OnHealthPickup(Val);
		break;
	}
}

void AGroceryRunCharacter::OnSpeedPickup(float Val){
	GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed + Val;
	GetCharacterMovement()->MaxWalkSpeedCrouched = BaseMoveSpeedCrouched + Val;
	HasSpeedBoost = true;
}

void AGroceryRunCharacter::ResetSpeed(){
	GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = BaseMoveSpeedCrouched;
}

void AGroceryRunCharacter::OnHealthPickup(float Val)
{
}

