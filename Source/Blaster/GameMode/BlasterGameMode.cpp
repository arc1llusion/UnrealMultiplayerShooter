// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
	ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	if(EliminatedCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("Game Mode Character Eliminated"));
		EliminatedCharacter->Eliminate();
	}
}
