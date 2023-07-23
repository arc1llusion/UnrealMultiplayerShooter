// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void PlayerEliminated(ABlasterCharacter* EliminatedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController);

	virtual void RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController);

	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

private:	
	AActor* GetRespawnPointWithLargestMinimumDistance() const;
	float GetMinimumDistance(const AActor* SpawnPoint,  const TArray<AActor*>& Players) const;

	void BroadcastDefeat(const ABlasterPlayerState* Attacker, const ABlasterPlayerState* Victim) const;

	UPROPERTY(EditDefaultsOnly, Category = "Character Select")
	TMap<int32, UClass*> PawnTypes;
};
