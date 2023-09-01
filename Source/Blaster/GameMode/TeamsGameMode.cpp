// Fill out your copyright notice in the Description page of Project Settings.


#include "TeamsGameMode.h"

#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Kismet/GameplayStatics.h"

ATeamsGameMode::ATeamsGameMode()
{
	bIsTeamsMatch = true;
}

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

float ATeamsGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	const float CurrentDamage =  Super::CalculateDamage(Attacker, Victim, BaseDamage);

	if(Attacker == nullptr || Victim == nullptr)
	{
		return CurrentDamage;
	}

	const auto AttackerPlayerState = Attacker->GetPlayerState<ABlasterPlayerState>();
	const auto VictimPlayerState = Victim->GetPlayerState<ABlasterPlayerState>();

	if(AttackerPlayerState == nullptr || VictimPlayerState == nullptr)
	{
		return CurrentDamage;
	}

	if(VictimPlayerState == AttackerPlayerState)
	{
		return CurrentDamage;
	}

	if(AttackerPlayerState->GetTeam() == VictimPlayerState->GetTeam())
	{
		return 0;
	}

	return CurrentDamage;
}

void ATeamsGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
	ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	Super::PlayerEliminated(EliminatedCharacter, VictimController, AttackerController);

	if(AttackerController == nullptr || VictimController == nullptr)
	{
		return;
	}

	BlasterGameState = BlasterGameState == nullptr ? Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this)) : BlasterGameState;
	const auto AttackerPlayerState = Cast<ABlasterPlayerState>(AttackerController->PlayerState);
	
	if(BlasterGameState && AttackerPlayerState)
	{
		if(AttackerPlayerState->GetTeam() == ETeam::ET_BlueTeam)
		{
			BlasterGameState->AddToBlueTeamScore();
		}

		if(AttackerPlayerState->GetTeam() == ETeam::ET_RedTeam)
		{
			BlasterGameState->AddToRedTeamScore();
		}
	}
}
