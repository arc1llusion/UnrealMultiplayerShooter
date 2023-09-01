// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameState.h"

#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Net/UnrealNetwork.h"


void ABlasterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterGameState, TopScoringPlayers);
	DOREPLIFETIME(ABlasterGameState, RedTeamScore);
	DOREPLIFETIME(ABlasterGameState, BlueTeamScore);
}

void ABlasterGameState::UpdateTopScore(ABlasterPlayerState* ScoringPlayer)
{
	if(TopScoringPlayers.Num() == 0)
	{
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
	else if(ScoringPlayer->GetScore() == TopScore)
	{
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}
	else if (ScoringPlayer->GetScore() > TopScore)
	{
		TopScoringPlayers.Empty();
		TopScoringPlayers.AddUnique(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
}

void ABlasterGameState::RemovePlayer(ABlasterPlayerState* LeavingPlayer)
{
	if(LeavingPlayer && TopScoringPlayers.Contains(LeavingPlayer))
	{
		TopScoringPlayers.Remove(LeavingPlayer);
	}
}

void ABlasterGameState::GetTopScoringPlayers(TArray<ABlasterPlayerState*>& OutTopScoringPlayers)
{
	OutTopScoringPlayers.Empty();

	for(auto State : TopScoringPlayers)
	{
		OutTopScoringPlayers.AddUnique(State);
	}
}

void ABlasterGameState::AddToRedTeamScore()
{
	++RedTeamScore;
}

void ABlasterGameState::OnRep_RedTeamScore()
{
	
}

void ABlasterGameState::AddToBlueTeamScore()
{
	++BlueTeamScore;
}

void ABlasterGameState::OnRep_BlueTeamScore()
{
}

void ABlasterGameState::AddPlayerToBlueTeam(ABlasterPlayerState* Player)
{
	if(Player)
	{
		BlueTeam.AddUnique(Player);
	}
}

void ABlasterGameState::AddPlayerToRedTeam(ABlasterPlayerState* Player)
{
	if(Player)
	{
		RedTeam.AddUnique(Player);
	}
}

void ABlasterGameState::RemovePlayerFromTeam(ABlasterPlayerState* Player)
{
	if(Player == nullptr)
	{
		return;
	}
	
	if(RedTeam.Contains(Player))
	{
		RedTeam.Remove(Player);
	}
	else if(BlueTeam.Contains(Player))
	{
		 BlueTeam.Remove(Player);
	}
}
