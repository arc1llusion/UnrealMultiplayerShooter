// Fill out your copyright notice in the Description page of Project Settings.


#include "CaptureTheFlagGameMode.h"

#include "Blaster/CaptureTheFlag/FlagZone.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/Weapon/Flag.h"

void ACaptureTheFlagGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
                                               ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	ABlasterGameMode::PlayerEliminated(EliminatedCharacter, VictimController, AttackerController);
}

void ACaptureTheFlagGameMode::FlagCaptured(const AFlag* Flag, const AFlagZone* Zone)
{
	BlasterGameState = BlasterGameState == nullptr ? GetGameState<ABlasterGameState>() : BlasterGameState;

	if(Flag && Zone && BlasterGameState && Flag->GetTeam() != Zone->GetTeam())
	{
		if(Zone->GetTeam() == ETeam::ET_BlueTeam)
		{
			BlasterGameState->AddToBlueTeamScore();
		}
		else if (Zone->GetTeam() == ETeam::ET_RedTeam)
		{
			BlasterGameState->AddToRedTeamScore();
		}
	}
}
