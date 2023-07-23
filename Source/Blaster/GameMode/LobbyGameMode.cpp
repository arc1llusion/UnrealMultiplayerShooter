// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/GameInstance/BlasterGameInstance.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	// if(GameState)
	// {
	// 	if(const int32 NumberOfPlayers = GameState->PlayerArray.Num(); NumberOfPlayers == 2)
	// 	{
	// 		if(UWorld* World = GetWorld())
	// 		{
	// 			//bUseSeamlessTravel = true;
	// 			World->ServerTravel(FString(TEXT("/Game/Maps/BlasterMap?listen")));
	// 		}
	// 	}
	// }
}

void ALobbyGameMode::GoToMainLevel()
{
	if(UWorld* World = GetWorld())
	{
		//bUseSeamlessTravel = true;
		World->ServerTravel(FString(TEXT("/Game/Maps/BlasterMap?listen")));
	}
}

UClass* ALobbyGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if(const auto BlasterPlayerController = Cast<ABlasterPlayerController>(InController))
	{
		if(PawnTypes.Contains(BlasterPlayerController->GetDesiredPawn()))
		{
			return PawnTypes[BlasterPlayerController->GetDesiredPawn()];
		}
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void ALobbyGameMode::RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController)
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

AActor* ALobbyGameMode::GetRespawnPointWithLargestMinimumDistance() const
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

	UE_LOG(LogTemp, Warning, TEXT("Largest Minimum Distance Found: %f"), LargestMinimumDistance);

	return SelectedSpawnPoint;
}

float ALobbyGameMode::GetMinimumDistance(const AActor* SpawnPoint, const TArray<AActor*>& Players) const
{
	float Minimum = UE_BIG_NUMBER;
	for(const AActor* Player : Players)
	{
		const float Distance = FVector::DistSquared(SpawnPoint->GetActorLocation(), Player->GetActorLocation());
		Minimum = FMath::Min(Distance, Minimum);
	}

	return FMath::Sqrt(Minimum);
}
