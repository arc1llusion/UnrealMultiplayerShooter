// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "BlasterGameState.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameState : public AGameState
{
	GENERATED_BODY()
	
public:
	virtual void AddToDefeatsLog(const FString& Defeated, const FString& DefeatedBy);

	UFUNCTION()
	virtual void OnRep_DefeatsLog();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
	
	UPROPERTY(ReplicatedUsing = OnRep_DefeatsLog)
	TArray<FString> DefeatsLog;

	UFUNCTION()
	void PruneDefeatsLog();

	UFUNCTION()
	void BroadcastDefeatsLog();

	TArray<FTimerHandle> Timers;
};
