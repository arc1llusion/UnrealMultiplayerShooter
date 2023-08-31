// Fill out your copyright notice in the Description page of Project Settings.


#include "TeamsGameMode.h"

#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Kismet/GameplayStatics.h"

void ATeamsGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	SortPlayerIntoTeam(NewPlayer->GetPlayerState<ABlasterPlayerState>());
}

void ATeamsGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	BlasterGameState = BlasterGameState == nullptr ? Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this)) : BlasterGameState;
	if(BlasterGameState)
	{
		BlasterGameState->RemovePlayerFromTeam(Exiting->GetPlayerState<ABlasterPlayerState>());	
	}	
}

void ATeamsGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	BlasterGameState = BlasterGameState == nullptr ? Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this)) : BlasterGameState;
	if(BlasterGameState)
	{
		for(APlayerState* PlayerState : BlasterGameState->PlayerArray)
		{
			SortPlayerIntoTeam(Cast<ABlasterPlayerState>(PlayerState));
		}
	}
}

void ATeamsGameMode::SortPlayerIntoTeam(ABlasterPlayerState* BlasterPlayerState)
{
	if(BlasterPlayerState == nullptr)
	{
		return;
	}

	BlasterGameState = BlasterGameState == nullptr ? Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this)) : BlasterGameState;
	if(BlasterGameState)
	{
		if(BlasterPlayerState && BlasterPlayerState->GetTeam() == ETeam::ET_NoTeam)
		{
			if(BlasterGameState->GetBlueTeamCount() >= BlasterGameState->GetRedTeamCount())
			{
				BlasterGameState->AddPlayerToRedTeam(BlasterPlayerState);
				BlasterPlayerState->SetTeam(ETeam::ET_RedTeam);
			}
			else
			{
				BlasterGameState->AddPlayerToBlueTeam(BlasterPlayerState);
				BlasterPlayerState->SetTeam(ETeam::ET_BlueTeam);
			}
		}
	}
}
