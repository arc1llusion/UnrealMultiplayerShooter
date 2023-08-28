#include "BlasterGameModeBase.h"

// Fill out your copyright notice in the Description page of Project Settings.


#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

void ABlasterGameModeBase::RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController, bool bRestartAtTransform, const FTransform& Transform)
{
	if(EliminatedCharacter)
	{
		EliminatedCharacter->Reset();
		EliminatedCharacter->Destroy();
	}

	if(EliminatedController)
	{
		if(!bRestartAtTransform)
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
		else
		{
			RestartPlayerAtTransform(EliminatedController, Transform);
		}
	}
}

AActor* ABlasterGameModeBase::GetRespawnPointWithLargestMinimumDistance() const
{
	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
	
	TArray<AActor*> Players;
	UGameplayStatics::GetAllActorsOfClass(this, ABlasterCharacter::StaticClass(), Players);

	if(PlayerStarts.Num() == 0)
	{
		return nullptr;
	}

	if(Players.Num() == 0)
	{
		//Choose any
		return PlayerStarts[FMath::RandRange(0, PlayerStarts.Num() - 1)];
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

	return SelectedSpawnPoint;
}

float ABlasterGameModeBase::GetMinimumDistance(const AActor* SpawnPoint, const TArray<AActor*>& Players) const
{
	float Minimum = UE_BIG_NUMBER;
	for(const AActor* Player : Players)
	{
		const float Distance = FVector::DistSquared(SpawnPoint->GetActorLocation(), Player->GetActorLocation());
		Minimum = FMath::Min(Distance, Minimum);
	}

	return FMath::Sqrt(Minimum);
}

void ABlasterGameModeBase::PlayerLeftGame(ABlasterPlayerState* PlayerLeaving)
{
	if(PlayerLeaving == nullptr)
	{
		return;
	}
	
	if(ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>())
	{
		BlasterGameState->RemovePlayer(PlayerLeaving);
	}

	if(const auto CharacterLeaving = Cast<ABlasterCharacter>(PlayerLeaving->GetPawn()))
	{
		CharacterLeaving->Eliminate(true);
	}
}