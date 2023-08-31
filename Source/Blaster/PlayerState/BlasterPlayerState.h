// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/BlasterTypes/Team.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

class ABlasterPlayerController;
class ABlasterCharacter;

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void AddToScore(float ScoreAmount);
	void AddToDefeats(int32 DefeatsAmount);
	void UpdateHUDDefeatsLog(const TArray<FString>& DefeatsLog);

	/*
	 * Replication Notifies
	 */
	virtual void OnRep_Score() override;

	UFUNCTION()
	virtual void OnRep_Defeats();

	FORCEINLINE ETeam GetTeam() const { return Team; }
	void SetTeam(const ETeam InTeam);

private:
	UPROPERTY()
	ABlasterCharacter* Character;
	UPROPERTY()
	ABlasterPlayerController* Controller;

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats;

	UPROPERTY(ReplicatedUsing = OnRep_Team)
	ETeam Team = ETeam::ET_NoTeam;

	UFUNCTION()
	void OnRep_Team();
};
