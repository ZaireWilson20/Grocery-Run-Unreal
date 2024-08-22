// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GRKarenState.h"
#include "UObject/Object.h"

#include "KarenStateMachine.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTransitionValidationCheck);
/**
 * 
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GROCERYRUN_API UKarenStateMachine : public UActorComponent
{
	GENERATED_BODY()

public:
	UKarenStateMachine();
	void AddTransition(UGRStateDataAsset* from, UGRStateDataAsset* to, const FValidationDelegate& validationFunction);
	UFUNCTION(BlueprintCallable)
	UTransition* AddTransition(UGRStateDataAsset* from, UGRStateDataAsset* to);
	void BindStateUpdateDelegate(UGRStateDataAsset* state,const TScriptDelegate<>& enterFunc);
	void BindStateEnterDelegate(UGRStateDataAsset* state, const TScriptDelegate<>& enterFunc);
	void BindStateExitDelegate(UGRStateDataAsset* state, const TScriptDelegate<>& exitFunc);
	void CheckForTransition();
	void TickFSM(float DeltaTime);
	void AddStateToObjMap(UGRStateDataAsset* state);
	void UpdateStateObjMap();
	void SetOwningActor(AActor* actor){OwningActor = actor;};

	AActor* GetOwningActor() const {return OwningActor;};
	virtual void BeginPlay() override;

	void InitializeOnPlay();

	
	UFUNCTION(BlueprintCallable)
	UGrKarenState* GetStateObject(UGRStateDataAsset* dataAsset){return StatesObjMap.Contains(dataAsset) ? StatesObjMap[dataAsset] : nullptr;}
	UFUNCTION(BlueprintCallable)
	void ValidateTransition(UTransitionData* transitionData, bool validate);
	UFUNCTION(BlueprintCallable)
	bool ShouldValidateTransition(UTransitionData* transitionData);

	
#if WITH_EDITOR
	
	// Called when a property is edited in the Unreal Editor
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditUndo() override;

	UFUNCTION()
	void RefreshStateDataAssetMap();
#endif
	
	UGrKarenState* GetCurrentState() const { return CurrentState; }
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="StateContainer")
	TSet<UGRStateDataAsset*> States;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="StateContainer")
	TMap<UGRStateDataAsset*,UGrKarenState*> StatesObjMap;
	UPROPERTY(EditAnywhere, Category="StateContainer")
	TMap<UGRStateDataAsset*, bool> DataAssetEditSubscriptionMap;
	UPROPERTY(EditAnywhere, Category="StateContainer")
	UGRStateDataAsset* InitialState;
	UPROPERTY(EditAnywhere, Category="StateContainer")
	AActor* OwningActor;
	UPROPERTY(EditAnywhere, Category="StateContainer")
	TSet<UGRStateDataAsset*> PreviousStates;
	UPROPERTY(BlueprintAssignable, Category="Events")
	FTransitionValidationCheck OnTransitionValidationCheck;
	
private:
	UPROPERTY(VisibleAnywhere)
	UGrKarenState* CurrentState;

};
