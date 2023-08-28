// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameState.h"

#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Net/UnrealNetwork.h"


void ABlasterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterGameState, DefeatsLog);
	DOREPLIFETIME(ABlasterGameState, TopScoringPlayers);
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

void ABlasterGameState::AddToDefeatsLog(const FString& Defeated, const FString& DefeatedBy)
{
	DefeatsLog.Add(FString::Printf(TEXT("%s eliminated by %s"), *Defeated, *DefeatedBy));
	BroadcastDefeatsLog();

	if(HasAuthority())
	{		
		FTimerHandle NewTimer;
		GetWorldTimerManager().SetTimer(NewTimer, this, &ABlasterGameState::PruneDefeatsLog, 5.0f);

		Timers.Add(NewTimer);
	}
}

void ABlasterGameState::AddToDefeatsLogFell(const FString& Fell)
{
	DefeatsLog.Add(FString::Printf(TEXT("%s fell!"), *Fell));
	BroadcastDefeatsLog();

	if(HasAuthority())
	{		
		FTimerHandle NewTimer;
		GetWorldTimerManager().SetTimer(NewTimer, this, &ABlasterGameState::PruneDefeatsLog, 5.0f);

		Timers.Add(NewTimer);
	}
}

void ABlasterGameState::OnRep_DefeatsLog()
{
	BroadcastDefeatsLog();
}

void ABlasterGameState::GetTopScoringPlayers(TArray<ABlasterPlayerState*>& OutTopScoringPlayers)
{
	OutTopScoringPlayers.Empty();

	for(auto State : TopScoringPlayers)
	{
		OutTopScoringPlayers.AddUnique(State);
	}
}

void ABlasterGameState::PruneDefeatsLog()
{
	if(DefeatsLog.Num() > 0)
	{
		DefeatsLog.RemoveAt(0);
	}

	if(Timers.Num() > 0)
	{
		FTimerHandle TimerToRemove = Timers[0];
		Timers.RemoveAt(0);
		GetWorldTimerManager().ClearTimer(TimerToRemove);
	}

	if(HasAuthority())
	{
		BroadcastDefeatsLog();
	}
}

void ABlasterGameState::BroadcastDefeatsLog()
{
	for(auto Player : PlayerArray)
	{
		if(const auto State = Cast<ABlasterPlayerState>(Player))
		{
			State->UpdateHUDDefeatsLog(DefeatsLog);
		}
	}
}
