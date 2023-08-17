// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasterGameModeBase.h"
#include "LobbyGameMode.generated.h"

class ABlasterPlayerController;
/**
 * 
 */
UCLASS()
class BLASTER_API ALobbyGameMode : public ABlasterGameModeBase
{
	GENERATED_BODY()

public:
	virtual void PostLogin(APlayerController* NewPlayer) override;

	UFUNCTION(BlueprintCallable)
	void GoToMainLevel();

	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	void SetPlayerReady(ABlasterPlayerController* PlayerController);

protected:
	virtual void Tick(float DeltaSeconds) override;
	
private:
	TMap<ABlasterPlayerController*, bool> PlayersReady;

	bool AreAllPlayersReady();
};
