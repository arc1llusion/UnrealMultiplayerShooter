// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasterGameModeBase.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"


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
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	FORCEINLINE float GetWarmupTime() const { return WarmupTime; }

protected:
	virtual void BeginPlay() override;
	
private:	
	void BroadcastDefeat(const ABlasterPlayerState* Attacker, const ABlasterPlayerState* Victim) const;

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.0f;

	float LevelStartingTime = 0.0f;

	float CountdownTime = 0.0f;
};
