// Fill out your copyright notice in the Description page of Project Settings.


#include "GroceryRun/Public/KarenCharacter.h"

#include "AIController.h"
#include "GRKarenChaseState.h"
#include "GRKarenFallenState.h"
#include "GRKarenIdleState.h"
#include "VectorTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "GroceryRun/GroceryRunCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "KarenStateMachine.h"


class AAIController;


float HorizontalDistance(FVector a, FVector b){
	return UE::Geometry::Distance(FVector(a.X, a.Y, 0), FVector(b.X, b.Y, 0));
}

// Sets default values
AKarenCharacter::AKarenCharacter()
{
	//KarenStateMachine = CreateDefaultSubobject<UKarenStateMachine>(TEXT("State Mahcine"));
	//KarenStateMachine->SetOwningActor(this);
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	//UE_LOG(LogTemp, Warning, TEXT("Karen Character Constructor"));

	PrimaryActorTick.bCanEverTick = true;
	CurrentMoveState = ENPCState::Idle;
	if(PlayerCharacter){
		//Note: function parameters not supported
		MyScriptDelegate.BindUFunction(this, "OnPushedByPlayer");
		PlayerCharacter->PushCollider->OnComponentBeginOverlap.AddUnique(MyScriptDelegate);
	}
}

bool AKarenCharacter::IdleToChase(const AKarenCharacter* KarenCharacter)
{
	UE_LOG(LogTemp, Warning, TEXT("Idle To Chase Called"));
	bool valid = KarenCharacter != nullptr;
	if(valid) valid = KarenCharacter->IdleToChaseDebug;

	UE_LOG(LogTemp, Warning, TEXT("Idle To Chase Called: Passes? %d"), valid);

	return valid;
}

// Called when the game starts or when spawned
void AKarenCharacter::BeginPlay()
{
	Super::BeginPlay();
	StateMachine = Cast<UKarenStateMachine>(GetComponentByClass(UKarenStateMachine::StaticClass()));

	if(StateMachine != nullptr)
	{
		StateMachine->InitializeOnPlay();

		/*KarenStateMachine->AddStateToObjMap(IdleState);
		KarenStateMachine->AddStateToObjMap(ChaseState);
		KarenStateMachine->AddStateToObjMap(FallenState);*/


		// Set Idle Transitions
		FValidationDelegate IdleValidDelegate;

		/*
		IdleValidDelegate.BindUFunction(this, "IdleToChase");
		KarenStateMachine->AddTransition(IdleState, ChaseState, IdleValidDelegate);*/
	}



	
	if(LinearMinuteTimeCurve) {
		FOnTimelineFloat chaseFloat;
		chaseFloat.BindUFunction(this, FName("HandleStopChaseTimer"));
		StopChaseTimeline.AddInterpFloat(LinearMinuteTimeCurve, chaseFloat);

		FOnTimelineFloat crashOutFloat;
		crashOutFloat.BindUFunction(this, FName("HandleCrashOutTimer"));
		CrashOutTimeline.AddInterpFloat(LinearMinuteTimeCurve, crashOutFloat);

		FOnTimelineFloat patienceFloat;
		patienceFloat.BindUFunction(this, FName("HandlePatienceTimer"));
		PatienceTimeline.AddInterpFloat(LinearMinuteTimeCurve, patienceFloat);

		FOnTimelineFloat downedFloat;
		downedFloat.BindUFunction(this,  FName("HandleDownedTimer"));
		DownedTimeLine.AddInterpFloat(LinearMinuteTimeCurve, patienceFloat);
	}
	
	if(PlayerCharacter == nullptr) {
		PlayerCharacter = Cast<AGroceryRunCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
		if(!PlayerCharacter) {
			UE_LOG(LogTemp, Warning, TEXT("PlayerCharacter Cast Failed, Player Character on KarenCharacter is Null"));
		}
	}
	if(AiController == nullptr){
		AiController = Cast<AAIController>(GetController());
		AiController->ReceiveMoveCompleted.AddDynamic(this, &AKarenCharacter::OnMoveCompletedHandler);

	}
	
}

void AKarenCharacter::TransitionToPatrolAnimation(float DeltaTime){
	Timer += DeltaTime;
	
	float alpha = FMath::Clamp(Timer/MaxPatrolAnimTransitionTime, 0.0, 1.0);
	WalkingValue = FMath::Lerp(0.0, 1.0, alpha);
	if(Timer >= MaxPatrolAnimTransitionTime){
		AnimTransitioning = false;
		Timer = 0; 
	}


}

void AKarenCharacter::TransitionToIdleAnimation(float DeltaTime){
	Timer += DeltaTime;
	
	float alpha = FMath::Clamp(Timer/MaxIdleAnimTransitionTime, 0.0, 1.0);
	WalkingValue = FMath::Lerp(1.0, 0.0, alpha);

	if(Timer >= MaxIdleAnimTransitionTime){
		AnimTransitioning = false;
		Timer = 0; 
	}
}

void AKarenCharacter::ScrewWithPlayer() {
	FHitResult hit;
	FVector loc = GetActorLocation();
	FVector locEnd = loc + FVector(0, 0, 0);
	FQuat Rotation = FQuat::Identity;
	FCollisionShape sphereShape = FCollisionShape::MakeSphere(CurrentMoveState != ENPCState::Chase ? PatienceRadius : ChaseRadius);
	FCollisionQueryParams params;
	params.AddIgnoredActor(this);
	bool hitSomething = GetWorld()->SweepSingleByChannel(hit, GetActorLocation(), locEnd, Rotation, ECC_Pawn, sphereShape, params);
	bool hitPlayer = hitSomething && hit.GetActor()->IsA(AGroceryRunCharacter::StaticClass());

	if(CurrentMoveState != ENPCState::Chase) {
		if(hitPlayer) {
			if(!PatienceTimeline.IsPlaying()) {
				PatienceTimeline.PlayFromStart();
			}
			DrawDebugSphere(GetWorld(), locEnd, PatienceRadius, 12, FColor::Red, false, .15f);
		} else {
			if(PatienceTimeline.IsPlaying()) {
				PatienceTimeline.Stop();
			}
			DrawDebugSphere(GetWorld(), locEnd, PatienceRadius, 12, FColor::Green, false, .15f);
		}
	} else { // If player isn't in karen chase radius during chase, player should be able to break chase mode
		if(!hitPlayer) {

			// Start the chase exit timeline if's not already playing --> Ticks HandleStopChaseTimer()
			if(!StopChaseTimeline.IsPlaying()) {
				StopChaseTimeline.PlayFromStart();
			}

			// Stop crashout timeline since player isn't in trigger radius
			if(CrashOutTimeline.IsPlaying()) {
				CrashOutTimeline.Stop();
			}
			
			DrawDebugSphere(GetWorld(), locEnd, ChaseRadius, 12, FColor::Cyan, false, .15f);
		} else {

			// Stop chase exit timeline since player is in trigger radius
			if(StopChaseTimeline.IsPlaying()) {
				StopChaseTimeline.Stop();
			}

			// Start the crash out timeline if's not already playing --> Ticks HandleCrashOutTimer()
			if(!CrashOutTimeline.IsPlaying()) {
				CrashOutTimeline.PlayFromStart();
			}
			
			DrawDebugSphere(GetWorld(), locEnd, ChaseRadius, 12, FColor::Yellow, false, .15f);
		}
	}

}

void AKarenCharacter::HandlePatienceTimer(float Val) {
	if(CurrentMoveState == ENPCState::Chase || CurrentMoveState == ENPCState::Downed) { return; }
	
	if(Val >= TimeToTriggerChase) {
		PatienceTimeline.Stop();
		AiController->StopMovement();
		CurrentMoveState = ENPCState::Chase;
	}
}

void AKarenCharacter::HandleDownedTimer(float Val) {
	if(CurrentMoveState != ENPCState::Downed){
		return;
	}
	if(Val >= TimeToTriggerChase) {
		DownedTimeLine.Stop();
		ResetKaren();
	}
}


void AKarenCharacter::HandleStopChaseTimer(float Val) {
	if(CurrentMoveState == ENPCState::Idle || CurrentMoveState == ENPCState::Downed) { return; }
	
	if(Val >= TimeToTriggerExitChase) {
		ResetKaren();
	}
}

void AKarenCharacter::HandleCrashOutTimer(float Val) {
	if(CurrentMoveState == ENPCState::Downed){
		return;
	}
	if(Val >= TimeToTriggerCrashOut) {
		KarenedOut = true;
		ResetKaren();
		if(PlayerCharacter) {
			UE_LOG(LogTemp, Warning, TEXT("Karen Karend Out"));
			Cast<AGroceryRunCharacter>(PlayerCharacter)->LosePatience();
		}
	}
}

void AKarenCharacter::OnPushedByPlayer(){
	if(CurrentMoveState == ENPCState::Downed){
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("Pushed In Collision"));
	OnPushedEvent.Broadcast();
	AiController->StopMovement();
	DownedTimeLine.Play();
	PatienceTimeline.Stop();
	CrashOutTimeline.Stop();
	StopChaseTimeline.Stop();
	CurrentMoveState = ENPCState::Downed;
}




/**
 * Set Karen state to idle and stop current actions
 */
void AKarenCharacter::ResetKaren() {
	if(AiController) {
		AiController->StopMovement();
	}

	StopChaseTimeline.Stop();
	PatienceTimeline.Stop();
	CrashOutTimeline.Stop();
	
	Running = false;
	WaitTimer = 0;
	ShouldWaitNextMove = false;

	CurrentMoveState = ENPCState::Idle;
}

// Called every frame
void AKarenCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if(StateMachine)
	{
		StateMachine->TickFSM(DeltaTime);
	}

	if(KarenedOut) {
		return;
	}
	DownedTimeLine.TickTimeline(DeltaTime);
	if(CurrentMoveState == ENPCState::Downed){
		return;
	}
	PatienceTimeline.TickTimeline(DeltaTime);
	StopChaseTimeline.TickTimeline(DeltaTime);
	CrashOutTimeline.TickTimeline(DeltaTime);
	ScrewWithPlayer();

	if(AiController->GetMoveStatus() == EPathFollowingStatus::Moving && !MovingToLocation) {
		MovingToLocation = true;
		AnimTransitioning = true;
		Timer = 0;
	} else if (AiController->GetMoveStatus() == EPathFollowingStatus::Idle && MovingToLocation) {
		MovingToLocation = false;
		AnimTransitioning = true;
		Timer = 0;
	}
	
	if (AnimTransitioning && MovingToLocation) {
		TransitionToPatrolAnimation(DeltaTime);
	} else if (AnimTransitioning && !MovingToLocation) {
		TransitionToIdleAnimation(DeltaTime);
	}
	
	
	if(PatrolPoints.Num() > 0 && CurrentMoveState != ENPCState::Chase){
		if(AiController == nullptr || (CurrentMoveState == ENPCState::Idle && AiController->GetMoveStatus() == EPathFollowingStatus::Idle && !AnimTransitioning && !ShouldWaitNextMove)){
			MoveToNextPatrolPoint();
		} else if (ShouldWaitNextMove) {
			WaitBeforeMove(DeltaTime);
		}
	} else if (CurrentMoveState == ENPCState::Chase && PlayerCharacter) {
		FollowPlayer();
	}
	
}

// Called to bind functionality to input
void AKarenCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AKarenCharacter::OnMoveCompletedHandler(FAIRequestID RequestID, EPathFollowingResult::Type Result){
	if (Result == EPathFollowingResult::Success){
		UE_LOG(LogTemp, Warning, TEXT("Move completed successfully!"));
	} else if (Result == EPathFollowingResult::Blocked) {
		UE_LOG(LogTemp, Warning, TEXT("Move blocked!"));
	} else if (Result == EPathFollowingResult::OffPath) {
		UE_LOG(LogTemp, Warning, TEXT("Move off path!"));
	} else if (Result == EPathFollowingResult::Aborted) {
		UE_LOG(LogTemp, Warning, TEXT("Move aborted!"));
		return;
	}

	ShouldWaitNextMove = true;
	/*if(CurrentMoveState != ENPCState::Chase && CurrentMoveState != ENPCState::Downed) {
		AnimTransitioning = true;
		ShouldWaitNextMove = true;
		WaitTimer = 0;
		CurrentMoveState = ENPCState::Idle;
	}*/
}

void AKarenCharacter::WaitBeforeMove(float DeltaTime){
	WaitTimer += DeltaTime;
	
	if(WaitTimer >= MaxWaitTime){
		WaitTimer = 0;
		ShouldWaitNextMove = false;
		
	}
}

void AKarenCharacter::FollowPlayer() {
	if(!AiController) {
		UE_LOG(LogTemp, Warning, TEXT("AI Controller Null On Karen Follow Player!"));
		return;
	}
	if(CurrentMoveState != ENPCState::Chase)
	{
		return;
	}
	float distToPlayer = HorizontalDistance(GetActorLocation(), PlayerCharacter->GetActorLocation());
	float clamedDist = FMath::Clamp(distToPlayer - ChaseRadius, 0, ChaseRadius * .5f);
	float wlkSpeed = FMath::Lerp(100.0f, 200.0f, clamedDist/(ChaseRadius * .5f) );
	//UE_LOG(LogTemp, Warning, TEXT("Karen Dist to Player unclamped: %f -- WalkSpeed: %f -- alpha(dist/chase * .5f): %f"), distToPlayer, wlkSpeed, clamedDist/(ChaseRadius * .5f));
	
	if(wlkSpeed > 100.0f) {
		Running = true;
		GetCharacterMovement()->MaxWalkSpeed = wlkSpeed;
	} else {
		Running = false;
	}
	
	if(distToPlayer >= ChaseRadius) {
		AiController->MoveToActor(PlayerCharacter, ChaseRadius * .5f);
		UE_LOG(LogTemp, Warning, TEXT("Move to actor"));
	}
	
}


void AKarenCharacter::MoveToNextPatrolPoint(){
	if(KarenedOut) {
		return;
	}
	CurrentMoveState = ENPCState::Patrol;
	LastPatrolPoint = ChooseRandomPatrolPoint();
	AnimTransitioning = true;
	if(AiController == nullptr){
		AiController = Cast<AAIController>(GetController()->GetPawn()->GetController());
	}

	AiController->MoveToLocation(PatrolPoints[LastPatrolPoint]->GetActorLocation());
	UE_LOG(LogTemp, Warning, TEXT("Move started!"));
	IsPowerWalking = FMath::RandRange(0,1) == 1;
	
	if(IsPowerWalking){
		GetCharacterMovement()->Velocity *= RunSpeed;
	}
}

int AKarenCharacter::ChooseRandomPatrolPoint(){
	int minValue = 0;
	int maxValue = PatrolPoints.Num();
	int randomIntInRange = FMath::Clamp(FMath::RandRange(minValue, maxValue - 1), 0, maxValue - 1);

	if(randomIntInRange == LastPatrolPoint && maxValue != 1){
		if(randomIntInRange == maxValue - 1){
			randomIntInRange -= 1;
		} else {
			randomIntInRange += 1;
		}
	}

	return randomIntInRange;
}





