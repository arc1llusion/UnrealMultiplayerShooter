// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"

#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

void ALobbyGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if(AreAllPlayersReady())
	{
		GoToMainLevel();
	}
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if(ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(NewPlayer))
	{
		PlayersReady.Add(BlasterPlayerController, false);
	}

	for(const auto& Item : PlayersReady)
	{
		Item.Key->RegisterPlayersReady(PlayersReady);
	}
}

void ALobbyGameMode::GoToMainLevel()
{
	if(UWorld* World = GetWorld())
	{
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

void ALobbyGameMode::SetPlayerReady(ABlasterPlayerController* PlayerController)
{
	if(!PlayersReady.Contains(PlayerController))
	{
		PlayersReady.Add(PlayerController, true);
	}
	else
	{
		PlayersReady[PlayerController] = true;
	}

	for(const auto& Item : PlayersReady)
	{
		Item.Key->RegisterPlayersReady(PlayersReady);
	}
}

bool ALobbyGameMode::AreAllPlayersReady()
{
	bool bIsReady = true;

	for(const auto& Item : PlayersReady)
	{
		if(!Item.Value)
		{
			bIsReady = false;
			break;
		}
	}

	return bIsReady;
}
