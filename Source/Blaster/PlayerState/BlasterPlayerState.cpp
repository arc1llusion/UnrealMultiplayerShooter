// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerState.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Net/UnrealNetwork.h"

void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerState, Defeats);
	DOREPLIFETIME_CONDITION(ABlasterPlayerState, DefeatsLog, COND_OwnerOnly);
}

void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);

	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	if(Character)
	{
		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

		if(Controller)
		{
			Controller->SetHUDScore(GetScore());
		}
	}
}

void ABlasterPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	Defeats += DefeatsAmount;

	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	if(Character)
	{
		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

		if(Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
	}
}

void ABlasterPlayerState::AddToDefeatsLog(const FString& Defeated, const FString& DefeatedBy)
{
	DefeatsLog.Add(FString::Printf(TEXT("%s eliminated by %s"), *Defeated, *DefeatedBy));

	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	if(Character)
	{
		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

		if(Controller)
		{
			Controller->SetHUDDefeatsLog(DefeatsLog);
		}
	}

	if(HasAuthority())
	{		
		FTimerHandle NewTimer;
		GetWorldTimerManager().SetTimer(NewTimer, this, &ABlasterPlayerState::PruneDefeatsLog, 3.0f);

		Timers.Add(NewTimer);
	}
}

void ABlasterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();
	
	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	if(Character)
	{
		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

		if(Controller)
		{
			Controller->SetHUDScore(GetScore());
		}
	}
}

void ABlasterPlayerState::OnRep_Defeats()
{
	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	if(Character)
	{
		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

		if(Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
	}
}

void ABlasterPlayerState::OnRep_DefeatsLog()
{
	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	if(Character)
	{
		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

		if(Controller)
		{
			Controller->SetHUDDefeatsLog(DefeatsLog);
		}
	}
}

void ABlasterPlayerState::PruneDefeatsLog()
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

	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	if(HasAuthority() && Character)
	{
		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

		if(Controller)
		{
			Controller->SetHUDDefeatsLog(DefeatsLog);
		}
	}
}
