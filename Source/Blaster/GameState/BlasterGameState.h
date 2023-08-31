// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "BlasterGameState.generated.h"

class ABlasterPlayerState;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameState : public AGameState
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void UpdateTopScore(ABlasterPlayerState* ScoringPlayer);
	void RemovePlayer(ABlasterPlayerState* LeavingPlayer);

	virtual void GetTopScoringPlayers(TArray<ABlasterPlayerState*>& OutTopScoringPlayers);

	FORCEINLINE int32 GetBlueTeamCount() const { return BlueTeam.Num(); }
	FORCEINLINE int32 GetRedTeamCount() const { return RedTeam.Num(); }

	void AddPlayerToBlueTeam(ABlasterPlayerState* Player);
	void AddPlayerToRedTeam(ABlasterPlayerState* Player);

	void RemovePlayerFromTeam(ABlasterPlayerState* Player);
	
private:
	
	TArray<FTimerHandle> Timers;

	UPROPERTY(Replicated)
	TArray<ABlasterPlayerState*> TopScoringPlayers;
	
	float TopScore = 0.0f;

	TArray<ABlasterPlayerState*> RedTeam;
	TArray<ABlasterPlayerState*> BlueTeam;

	UPROPERTY(ReplicatedUsing = OnRep_RedTeamScore)
	float RedTeamScore = 0.0f;

	UFUNCTION()
	void OnRep_RedTeamScore();

	UPROPERTY(ReplicatedUsing = OnRep_BlueTeamScore)
	float BlueTeamScore = 0.0f;

	UFUNCTION()
	void OnRep_BlueTeamScore();
};
