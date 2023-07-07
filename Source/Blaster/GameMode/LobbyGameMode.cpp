// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"

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
