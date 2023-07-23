// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

class ABlasterCharacter;
/**
 * 
 */
UCLASS()
class BLASTER_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void PostLogin(APlayerController* NewPlayer) override;

	UFUNCTION(BlueprintCallable)
	void GoToMainLevel();

	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	virtual void RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController);

	FORCEINLINE int32 GetNumberOfPossibleCharacters() const { return PawnTypes.Num(); }
private:
	AActor* GetRespawnPointWithLargestMinimumDistance() const;
	float GetMinimumDistance(const AActor* SpawnPoint,  const TArray<AActor*>& Players) const;

	UPROPERTY(EditDefaultsOnly, Category = "Character Select")
	TMap<int32, UClass*> PawnTypes;
};
