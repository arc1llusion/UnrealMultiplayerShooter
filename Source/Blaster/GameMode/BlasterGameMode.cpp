// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
                                        ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;

	if(AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.0f);
	}

	if(VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	BroadcastDefeat(AttackerPlayerState, VictimPlayerState);
	
	if(EliminatedCharacter)
	{
		EliminatedCharacter->Eliminate();
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController)
{
	if(EliminatedCharacter)
	{
		EliminatedCharacter->Reset();
		EliminatedCharacter->Destroy();
	}

	if(EliminatedController)
	{
		if(AActor* SpawnPoint = GetRespawnPointWithLargestMinimumDistance())
		{
			RestartPlayerAtPlayerStart(EliminatedController, SpawnPoint);	
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("No player spawn point found."));
		}
	}
}

AActor* ABlasterGameMode::GetRespawnPointWithLargestMinimumDistance() const
{
	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
	
	TArray<AActor*> Players;
	UGameplayStatics::GetAllActorsOfClass(this, ABlasterCharacter::StaticClass(), Players);

	if(PlayerStarts.Num() == 0 || Players.Num() == 0)
	{
		return nullptr;
	}

	float LargestMinimumDistance = -UE_BIG_NUMBER;
	AActor* SelectedSpawnPoint = nullptr;
	for(AActor* SpawnPoint : PlayerStarts)
	{
		const float Distance = GetMinimumDistance(SpawnPoint, Players);
		if(Distance > LargestMinimumDistance)
		{
			LargestMinimumDistance = Distance;
			SelectedSpawnPoint = SpawnPoint;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Largest Minimum Distance Found: %f"), LargestMinimumDistance);

	return SelectedSpawnPoint;
}

float ABlasterGameMode::GetMinimumDistance(const AActor* SpawnPoint, const TArray<AActor*>& Players) const
{
	float Minimum = UE_BIG_NUMBER;
	for(const AActor* Player : Players)
	{
		const float Distance = FVector::DistSquared(SpawnPoint->GetActorLocation(), Player->GetActorLocation());
		Minimum = FMath::Min(Distance, Minimum);
	}

	return FMath::Sqrt(Minimum);
}

void ABlasterGameMode::BroadcastDefeat(const ABlasterPlayerState* Attacker, const ABlasterPlayerState* Victim) const
{
	if(!Attacker || !Victim)
	{
		return;
	}

	if(GameState)
	{
		for(const auto Player : GameState->PlayerArray)
		{
			if(const auto BlasterPlayerState = Cast<ABlasterPlayerState>(Player))
			{
				BlasterPlayerState->AddToDefeatsLog(Victim->GetPlayerName(), Attacker->GetPlayerName());
			}
		}
	}
}
