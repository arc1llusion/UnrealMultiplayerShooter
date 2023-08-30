// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/GameInstance/BlasterGameInstance.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "GameFramework/GameStateBase.h"

namespace MatchState
{
	const FName Cooldown = FName(TEXT("Cooldown"));
}

ABlasterGameMode::ABlasterGameMode()
{
	bDelayedStart = true;
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if(MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if(CountdownTime <= 0.0f)
		{
			StartMatch();
		}
	}
	else if(MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		
		if(CountdownTime <= 0.0f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if(CountdownTime <= 0.0f)
		{
			RestartGame();
		}
	}
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for(FConstPlayerControllerIterator PIt = GetWorld()->GetPlayerControllerIterator(); PIt; ++PIt)
	{
		if(ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(*PIt))
		{
			BlasterPlayer->OnMatchStateSet(MatchState);
		}
	}
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
                                        ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	if(!AttackerController || !AttackerController->PlayerState)
	{
		return;
	}

	if(!VictimController || !VictimController->PlayerState)
	{
		return;
	}
	
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;
	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();

	if(AttackerPlayerState && AttackerPlayerState != VictimPlayerState && BlasterGameState)
	{
		TArray<ABlasterPlayerState*> PreviousPlayersInLead;
		BlasterGameState->GetTopScoringPlayers(PreviousPlayersInLead);		
		
		AttackerPlayerState->AddToScore(1.0f);
		BlasterGameState->UpdateTopScore(AttackerPlayerState);

		TArray<ABlasterPlayerState*> UpdatedPlayersInLead;
		BlasterGameState->GetTopScoringPlayers(UpdatedPlayersInLead);

		HandleTopScoreChange(PreviousPlayersInLead, UpdatedPlayersInLead, AttackerPlayerState);
	}

	if(VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}
	
	if(EliminatedCharacter)
	{
		EliminatedCharacter->Eliminate(false);
	}

	for(FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(*It);
		if(BlasterPlayer && AttackerPlayerState && VictimPlayerState)
		{
			BlasterPlayer->BroadcastElimination(AttackerPlayerState, VictimPlayerState);
		}
	}
}

void ABlasterGameMode::PlayerFell(ABlasterCharacter* CharacterThatFell,
	ABlasterPlayerController* CharacterThatFellController)
{
	if(!CharacterThatFellController || !CharacterThatFellController->PlayerState)
	{
		return;
	}

	if(ABlasterPlayerState* CharacterThatFellPlayerState = CharacterThatFellController ? Cast<ABlasterPlayerState>(CharacterThatFellController->PlayerState) : nullptr)
	{
		CharacterThatFellPlayerState->AddToDefeats(1);		

		for(FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if(ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(*It))
			{
				BlasterPlayer->BroadcastFallElimination(CharacterThatFellPlayerState);
			}
		}
	}

	if(CharacterThatFell)
	{
		CharacterThatFell->Eliminate(false);
	}
}

UClass* ABlasterGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if(InController)
	{
		if(const auto BlasterGameInstance = Cast<UBlasterGameInstance>(GetGameInstance()))
		{
			if(InController->PlayerState)
			{
				const auto NetId = InController->PlayerState->GetUniqueId();
				if(NetId.IsValid())
				{
					const int32 SelectedPawnType = BlasterGameInstance->GetSelectedCharacter(NetId->GetHexEncodedString());
					if(PawnTypes.Contains(SelectedPawnType))
					{
						return PawnTypes[SelectedPawnType];
					}
				}
			}
		}
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void ABlasterGameMode::HandleTopScoreChange(const TArray<ABlasterPlayerState*>& PreviousTopScoringPlayers,
	const TArray<ABlasterPlayerState*>& CurrentTopScoringPlayers, const ABlasterPlayerState* ScoringPlayer) const
{
	if(ScoringPlayer == nullptr)
	{
		return;
	}
	
	if(CurrentTopScoringPlayers.Contains(ScoringPlayer))
	{
		if(const auto Leader = Cast<ABlasterCharacter>(ScoringPlayer->GetPawn()))
		{
			Leader->MulticastGainedTheLead();
		}
	}

	for(int32 i = 0; i < PreviousTopScoringPlayers.Num(); ++i)
	{
		if(!CurrentTopScoringPlayers.Contains(PreviousTopScoringPlayers[i]))
		{
			if(const auto Loser = Cast<ABlasterCharacter>(PreviousTopScoringPlayers[i]->GetPawn()))
			{
				Loser->MulticastLostTheLead();
			}
		}
	}
}
