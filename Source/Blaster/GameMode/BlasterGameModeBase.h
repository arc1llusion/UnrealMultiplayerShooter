#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameModeBase.generated.h"

class ABlasterPlayerState;
class ABlasterCharacter;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameModeBase : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController, bool bRestartAtTransform = false, const FTransform& Transform = FTransform::Identity);

	virtual void PlayerLeftGame(ABlasterPlayerState* PlayerLeaving);

	FORCEINLINE int32 GetNumberOfPossibleCharacters() const { return PawnTypes.Num(); }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Character Select")
	TMap<int32, UClass*> PawnTypes;
	
private:
	AActor* GetRespawnPointWithLargestMinimumDistance() const;
	float GetMinimumDistance(const AActor* SpawnPoint,  const TArray<AActor*>& Players) const;
};
