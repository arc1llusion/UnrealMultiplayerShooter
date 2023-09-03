// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"

#include "MultiplayerSessionsSubsystem.h"
#include "Blaster/GameInstance/BlasterGameInstance.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
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
		BlasterGameInstance = BlasterGameInstance == nullptr ? Cast<UBlasterGameInstance>(GetGameInstance()) : BlasterGameInstance;

		if(BlasterGameInstance)
		{
			MultiplayerSessionsSubsystem = MultiplayerSessionsSubsystem == nullptr ? BlasterGameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>() : MultiplayerSessionsSubsystem;
			check(MultiplayerSessionsSubsystem);


			const FString MatchType = MultiplayerSessionsSubsystem->GetDesiredMatchType();

			if(MatchType == TEXT("FreeForAll"))
			{
				World->ServerTravel(FString(TEXT("/Game/Maps/FreeForAll?listen")));
			}
			else if(MatchType == TEXT("Teams"))
			{
				World->ServerTravel(FString(TEXT("/Game/Maps/Teams?listen")));
			}
			else if(MatchType == TEXT("CaptureTheFlag"))
			{
				World->ServerTravel(FString(TEXT("/Game/Maps/CaptureTheFlag?listen")));
			}
		}
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

void ALobbyGameMode::PlayerLeftGame(ABlasterPlayerState* PlayerLeaving)
{
	if(const auto PlayerController = Cast<ABlasterPlayerController>(PlayerLeaving->GetPlayerController()))
	{		
		if(PlayersReady.Contains(PlayerController))
		{
			PlayersReady.Remove(PlayerController);
		}

		for(const auto& Item : PlayersReady)
		{
			Item.Key->RegisterPlayersReady(PlayersReady);
		}
	}
	
	Super::PlayerLeftGame(PlayerLeaving);
}
