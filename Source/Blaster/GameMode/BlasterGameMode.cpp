// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/GameInstance/BlasterGameInstance.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "GameFramework/GameStateBase.h"

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
                                        ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;

	if(AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.0f);
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
					int32 SelectedPawnType = BlasterGameInstance->GetSelectedCharacter(NetId->GetHexEncodedString());
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
