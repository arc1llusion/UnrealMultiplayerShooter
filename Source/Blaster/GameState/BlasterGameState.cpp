// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameState.h"

#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Net/UnrealNetwork.h"


void ABlasterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterGameState, DefeatsLog);
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

void ABlasterGameState::OnRep_DefeatsLog()
{
	BroadcastDefeatsLog();
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
