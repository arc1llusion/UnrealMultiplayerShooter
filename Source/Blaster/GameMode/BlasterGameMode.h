// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasterGameModeBase.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "BlasterGameMode.generated.h"

namespace MatchState
{
	extern BLASTER_API const FName Cooldown; //Match duration has been reached. Display winner and begin cooldown timer
}


class ABlasterPlayerState;
class ABlasterCharacter;
class ABlasterPlayerController;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public ABlasterGameModeBase
{
	GENERATED_BODY()

public:
	ABlasterGameMode();

	virtual void Tick(float DeltaSeconds) override;
	
	virtual void PlayerEliminated(ABlasterCharacter* EliminatedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController);
	virtual void PlayerFell(ABlasterCharacter* CharacterThatFell, ABlasterPlayerController* CharacterThatFellController);
	
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	FORCEINLINE float GetWarmupTime() const { return WarmupTime; }
	FORCEINLINE float GetCooldownTime() const { return CooldownTime; }
	FORCEINLINE float GetMatchTime() const { return MatchTime; }
	FORCEINLINE float GetLevelStartingTime() const { return LevelStartingTime; }
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }

protected:
	virtual void BeginPlay() override;

	virtual void OnMatchStateSet() override;
	
private:	
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.0f;

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.0f;

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.0f;

	float LevelStartingTime = 0.0f;

	float CountdownTime = 0.0f;

	void HandleTopScoreChange(const TArray<ABlasterPlayerState*>& PreviousTopScoringPlayers, const TArray<ABlasterPlayerState*>& CurrentTopScoringPlayers, const ABlasterPlayerState* ScoringPlayer) const;
};
