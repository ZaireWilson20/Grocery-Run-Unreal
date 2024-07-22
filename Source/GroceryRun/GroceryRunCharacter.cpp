// Copyright Epic Games, Inc. All Rights Reserved.

#include "GroceryRunCharacter.h"
#include "GroceryRunProjectile.h"
#include "Audio.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GroceryItemActor.h"
#include "InputActionValue.h"
#include "KarenCharacter.h"
#include "DSP/PerlinNoise.h"
#include "Engine/LocalPlayer.h"
#include "Field/FieldSystemNoiseAlgo.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"

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

	// Initialize sway parameters
	SwayAmplitude = .5f; // Adjust amplitude as needed
	SwayFrequency = .5f; // Adjust frequency as needed
	SwayTime = 0.0f;

	// Initialize Perlin noise parameters
	NoiseAmplitude = 70.0f; // Adjust amplitude as needed
	NoiseSpeed = .8f; // Adjust speed as needed
	NoiseTimeX = 0.0f;
	NoiseTimeY = 0.0f;
	InitialYaw = 0.0f;


	
}

void AGroceryRunCharacter::Tick(float DeltaTime){
	ShootCameraLineTrace();
	HandleSpeedPickupTiming(DeltaTime);
	HandleGroceryFocusTiming(DeltaTime);
	HandleFOVTransition(DeltaTime);
	CrouchTimeline.TickTimeline(DeltaTime);
	
	if (DynamicMat && !IsSetup){
		
		DynamicMat->SetScalarParameterValue(FName("Vignette_Strength"), 0);
		DynamicMat->SetScalarParameterValue(FName("Vignette_Zoom"), 0);

		IsSetup = true;
	}


	if(MovementLocked) {

		
		float cam = GetFirstPersonCameraComponent()->FieldOfView;
		bool betweenFOV = !FMath::IsNearlyEqual(cam, NormalFOV) || !FMath::IsNearlyEqual(cam, PickupFOV);
		NoiseTimeX += DeltaTime * NoiseSpeed;
		NoiseTimeY += DeltaTime * (NoiseSpeed/2);
		//if(!betweenFOV) {
			// Generate Perlin noise-based offsets within ±90 degrees
			float NoiseOffsetPitch = 20 * FMath::PerlinNoise1D(NoiseTimeX);
			float NoiseOffsetYaw = NoiseAmplitude * FMath::PerlinNoise1D(NoiseTimeY);

			// Apply combined sway and noise offsets to controller rotation
			//AddControllerYawInput(NoiseOffsetYaw);
			//AddControllerPitchInput(NoiseOffsetPitch * .5f);

			// Apply noise offsets to the initial rotation
			FRotator NewRotation = InitialRotation;
			NewRotation.Pitch = FMath::Clamp(NewRotation.Pitch - NoiseOffsetPitch + SmoothPitch * 40, InitialRotation.Pitch - 30.0f, InitialRotation.Pitch + 30.0f);
			NewRotation.Yaw = FMath::Clamp(NewRotation.Yaw + NoiseOffsetYaw + SmoothYaw * 40, InitialRotation.Yaw - 90.0f, InitialRotation.Yaw + 90.0f);

			if (APlayerController* PC = Cast<APlayerController>(GetController()))
			{
				PC->SetControlRotation(NewRotation);
			}
		SmoothYaw = FMath::Lerp(SmoothYaw, PrevControllerYaw, .2f * DeltaTime);
		SmoothPitch = FMath::Lerp(SmoothPitch, PrevControllerPitch, .2f * DeltaTime);
		//}
	}

}

void AGroceryRunCharacter::TriggerOnHoverEnter(){
	OnHoverEnter.Broadcast();
}

void AGroceryRunCharacter::TriggerOnHoverExit(){
	OnHoverExit.Broadcast();
}

void AGroceryRunCharacter::LosePatience() {
	if(Health > 0) {
		return;
	}	
	Health--;
	UE_LOG(LogTemp, Warning, TEXT("Player Lost Patience. New Health: %d"), Health);
}

void AGroceryRunCharacter::GainPatience() {
	if(Health > MaxHealth) {
		return;
	}
	Health++;
	UE_LOG(LogTemp, Warning, TEXT("Player Gain Patience. New Health: %d"), Health);

}


FVector2D AGroceryRunCharacter::GetCameraViewportCenter() const{
	FVector2D ViewportCenter(0.0f, 0.0f);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		int32 ViewportSizeX, ViewportSizeY;
		PC->GetViewportSize(ViewportSizeX, ViewportSizeY);

		// Calculate the center of the viewport
		ViewportCenter.X = ViewportSizeX * 0.5f;
		ViewportCenter.Y = ViewportSizeY * 0.5f;
	}

	return ViewportCenter;
}

void AGroceryRunCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	BaseMoveSpeed = GetCharacterMovement()->MaxWalkSpeed;
	BaseMoveSpeedCrouched = GetCharacterMovement()->MaxWalkSpeedCrouched;
	BaseCapHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();

	if(CrouchCurve) {
		FOnTimelineFloat CrouchProgFunc;

		CrouchProgFunc.BindUFunction(this, FName("HandleCrouchProgress"));
		CrouchTimeline.AddInterpFloat(CrouchCurve, CrouchProgFunc);
		
	}


}

void AGroceryRunCharacter::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp,
	bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit){
	
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	//if(Other->IsA(Pickup))
}

void AGroceryRunCharacter::HandleFOVTransition(float DeltaTime) {
	float cam = GetFirstPersonCameraComponent()->FieldOfView;
	bool betweenFOV = !FMath::IsNearlyEqual(cam, NormalFOV) || !FMath::IsNearlyEqual(cam, PickupFOV);
	bool toNorm = FMath::IsNearlyEqual(cam, PickupFOV) && !MovementLocked;
	bool toPickUp = FMath::IsNearlyEqual(cam, NormalFOV) && MovementLocked;
	
	if(betweenFOV || toNorm || toPickUp) {
		if (DynamicMat){
			float alphaStrength = FMath::Lerp(0, MaxVignetteStrength, FOV_TransitionTimer/FOV_Transition_time);
			float alphaZoom = FMath::Lerp(0, MaxVignetteZoom, FOV_TransitionTimer/FOV_Transition_time);
			DynamicMat->SetScalarParameterValue(FName("Vignette_Strength"), alphaStrength);
			DynamicMat->SetScalarParameterValue(FName("Vignette_Zoom"), alphaZoom);
		}
		FOV_TransitionTimer += (MovementLocked ? DeltaTime : -DeltaTime);
		FOV_TransitionTimer = FMath::Clamp(FOV_TransitionTimer, 0, FOV_Transition_time);
		float camFOV = FMath::Lerp(NormalFOV, PickupFOV, FOV_TransitionTimer/FOV_Transition_time);
		
		GetFirstPersonCameraComponent()->SetFieldOfView(FMath::Clamp(camFOV, ToPickupFOV, NormalFOV));
	}
}

void AGroceryRunCharacter::HandleCrouchProgress(float Val) {
	if(Crouching) {
		UCapsuleComponent* playerCap = GetCapsuleComponent();
		float curHeight = FMath::Lerp(BaseCapHeight, CrouchHeight,Val);
		playerCap->SetCapsuleHalfHeight(curHeight);
		if(curHeight <= CrouchHeight) {
			Crouched = true;
			Crouching = false;
			CrouchTimeline.Stop();
		}
	} else if(UnCrouching) {
		UCapsuleComponent* playerCap = GetCapsuleComponent();
		float curHeight = FMath::Lerp(BaseCapHeight, CrouchHeight,Val);
		Crouched = false;
		playerCap->SetCapsuleHalfHeight(curHeight);

		if(curHeight >= BaseCapHeight) {
			Crouched = false;
			UnCrouching = false;
			CrouchTimeline.Stop();
		}
	}
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

		// Grocery Lock
		EnhancedInputComponent->BindAction(LockAimAction, ETriggerEvent::Started, this, &AGroceryRunCharacter::LockOnGroceryStart);
		EnhancedInputComponent->BindAction(LockAimAction, ETriggerEvent::Completed, this, &AGroceryRunCharacter::LockOnGroceryEnd);

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

	FVector end = ((cameraLookDir * 100) + cameraCenterStart);

	FCollisionQueryParams colParams;
	colParams.AddIgnoredActor(this);
	FVector FakeStart = FVector(1560.0,360.0,100.0);
	FVector FakeEnd = (FVector(0,1,0) * 100) + FakeStart;
	FHitResult hit;
	bool hitObj = GetWorld()->LineTraceSingleByChannel(hit, cameraCenterStart, end, ECC_Visibility, colParams);

	UKismetSystemLibrary::DrawDebugLine(GetWorld(), FakeStart, FakeEnd, FColor(EVertexColorViewMode::Red));
	if(hitObj && hit.GetActor()->ActorHasTag("Focusable")){
		if(!lookingAtKaren){
			lookingAtKaren = true;
		}

		AGroceryItemActor* ItemActor = Cast<AGroceryItemActor>(hit.GetActor());
		if(!LookingAtGroceryItem && ItemActor){
			LookingAtGroceryItem = true;
			ActorInFocus = hit.GetActor();
			ItemInFocus = ItemActor->ItemData;
			if(MovementLocked) {
				LockOnGroceryStartManual();
			}
		}
		TriggerOnHoverEnter();
	} else {
		if(lookingAtKaren){
			lookingAtKaren = false;	

			TriggerOnHoverExit();
		}
		if(LookingAtGroceryItem){
			LookingAtGroceryItem = false;
			GroceryPickUpTimer = 0;
			TriggerOnHoverExit();
			PickingUpEnded.Broadcast();
		}
	}
	
}


#pragma region "Input Action"

void AGroceryRunCharacter::Jump() {
	if(Controller && !MovementLocked) {
		Super::Jump();
	}
}
void AGroceryRunCharacter::Move(const FInputActionValue& Value){

	if (Controller != nullptr && !MovementLocked){
		// input is a Vector2D
		FVector2D MovementVector = Value.Get<FVector2D>();
		
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AGroceryRunCharacter::LockOnGroceryStart(const FInputActionValue& Value){
	if(LookingAtGroceryItem && !MovementLocked){
		MovementLocked = true;
		GroceryPickUpTimer = 0;
		TriggerFOVTransition = true;
		ToPickupFOV = true;
		PickingUpStarted.Broadcast();
	}
	if (APlayerController* PC = Cast<APlayerController>(GetController())){
		InitialRotation = PC->GetControlRotation();
	}
}

void AGroceryRunCharacter::LockOnGroceryStartManual(){
	GroceryPickUpTimer = 0;
	PickingUpStarted.Broadcast();
}

void AGroceryRunCharacter::LockOnGroceryEnd(const FInputActionValue& Value){
	MovementLocked = false;
	// Potentially add some leniency -- i.e. if Grocery Timer is within some small range around 0 (GroceryPickUpTimer <= .1f)
	if(LookingAtGroceryItem){
		GroceryPickUpTimer = 0;
		ToPickupFOV = false;
		PickingUpEnded.Broadcast();
	}

}

void AGroceryRunCharacter::Look(const FInputActionValue& Value){
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	
	if (Controller != nullptr && !MovementLocked)
	{
		// add yaw and pitch input to controller
		//AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);

		FRotator NewRotation = Controller->GetControlRotation();
		NewRotation.Pitch = NewRotation.Pitch + LookAxisVector.Y;
		NewRotation.Yaw = NewRotation.Yaw + LookAxisVector.X;

		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->SetControlRotation(NewRotation);
		}
	}
	PrevControllerPitch = LookAxisVector.Y * -1;
	PrevControllerYaw = LookAxisVector.X;
}

void AGroceryRunCharacter::CharacterCrouch(const FInputActionValue& Value){
	if(Controller != nullptr && !MovementLocked && (!Crouched || !Crouching)){
		Crouching = true;
		UnCrouching = false;
		CrouchTimeline.Play();
	}
}

void AGroceryRunCharacter::CharacterUnCrouch(const FInputActionValue& Value){
	
	if(Controller != nullptr && (Crouched || Crouching)){
		UnCrouching = true;
		Crouching = false;
		CrouchTimeline.Reverse();
	}
}

#pragma endregion

void AGroceryRunCharacter::PickUpModifier(EPickupType Pickup, float Val){
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
	RunningStarted.Broadcast();
}

void AGroceryRunCharacter::ResetSpeed(){
	GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = BaseMoveSpeedCrouched;
}

void AGroceryRunCharacter::HandleGroceryFocusTiming(float DeltaTime){
	if(MovementLocked && LookingAtGroceryItem){
		if(GroceryPickUpTimer >= MaxGroceryPickUpTimer){
			MovementLocked = false;
			GroceryPickUpTimer = 0;


			if(ItemInFocus && ActorInFocus){
				BagInventory.Add(FGroceryInventoryItem(ItemInFocus));
				ActorInFocus->SetActorHiddenInGame(true);
				ActorInFocus->SetActorEnableCollision(ECollisionEnabled::NoCollision);
			}

			if(!ActorInFocus) {
				//UE_LOG(LogTemp, Warning, TEXT("Actor In Focus Null on Timer Done"));
			}			
			if(!ItemInFocus) {
				//UE_LOG(LogTemp, Warning, TEXT("Item In Focus Null on Timer Done"));
			}
			PickingUpEnded.Broadcast();
		} else {
			GroceryPickUpTimer += DeltaTime;
		}
	}
}

void AGroceryRunCharacter::HandleSpeedPickupTiming(float DeltaTime) {
	if(HasSpeedBoost){
		if(SpeedBoostTimer >= MaxSpeedBoostTime){
			ResetSpeed();
			RunningEnded.Broadcast();
			HasSpeedBoost = false;
			SpeedBoostTimer = 0;
		} else {
			SpeedBoostTimer += DeltaTime;
		}
	}
}

void AGroceryRunCharacter::OnHealthPickup(float Val)
{
}

