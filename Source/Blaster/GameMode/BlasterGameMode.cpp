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
		AttackerPlayerState->AddToScore(1.0f);
		BlasterGameState->UpdateTopScore(AttackerPlayerState);
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

void ABlasterGameMode::BroadcastDefeat(const ABlasterPlayerState* Attacker, const ABlasterPlayerState* Victim) const
{
	if(!Attacker || !Victim)
	{
		return;
	}

	if(GameState)
	{
		if(const auto BlasterGameState = Cast<ABlasterGameState>(GameState))
		{
			BlasterGameState->AddToDefeatsLog(Victim->GetPlayerName(), Attacker->GetPlayerName());
		}
	}
}
