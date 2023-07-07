// Fill out your copyright notice in the Description page of Project Settings.


#include "OverheadWidget.h"

#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"

void UOverheadWidget::SetDisplayText(const FString& TextToDisplay) const
{
	if(DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverheadWidget::ShowPlayerRemoteNetRole(APawn* InPawn)
{
	if(!InPawn)
	{
		return;
	}
	
	const ENetRole RemoteRole = InPawn->GetRemoteRole();

	FText EnumText;
	UEnum::GetDisplayValueAsText(RemoteRole, EnumText);

	const FString EnumString = EnumText.ToString().Replace(TEXT("Role "), TEXT(""));
	const FString Output = FString::Printf(TEXT("Remote Role: %s"), *EnumString);
	SetDisplayText(Output);
}

void UOverheadWidget::ShowPlayerLocalNetRole(APawn* InPawn)
{
	if(!InPawn)
	{
		return;
	}
	
	const ENetRole LocalRole = InPawn->GetLocalRole();

	FText EnumText;
	UEnum::GetDisplayValueAsText(LocalRole, EnumText);

	const FString EnumString = EnumText.ToString().Replace(TEXT("Role "), TEXT(""));
	const FString Output = FString::Printf(TEXT("Local Role: %s"), *EnumString);
	SetDisplayText(Output);
}

void UOverheadWidget::ShowPlayerName(APawn* InPawn)
{
	if(!InPawn)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if(!World)
	{
		return;
	}
	
	FTimerManager &TimerManager = World->GetTimerManager();	
	const APlayerState* PlayerState = InPawn->GetPlayerState();
	
	if(!PlayerState && TotalTime < PlayerStateTimerTimeout)
	{
		BindAndTickPlayerStateTimer(TimerManager, InPawn);
		return; //Once the timer is set we don't need to do anything else
	}

	ClearPlayerStateTimer(TimerManager);	

	if(PlayerState)
	{
		SetDisplayText(PlayerState->GetPlayerName());
	}
}

void UOverheadWidget::BindAndTickPlayerStateTimer(FTimerManager& TimerManager, APawn* InPawn)
{
	if(!PlayerStateTimerDelegate.IsBound())
	{
		PlayerStateTimerDelegate.BindUFunction(this, TEXT("ShowPlayerName"), InPawn);
	}

	if(PlayerStateTimerHandle.IsValid())
	{
		TimerManager.ClearTimer(PlayerStateTimerHandle);
	}
		
	TimerManager.SetTimer(PlayerStateTimerHandle, PlayerStateTimerDelegate, PlayerStateTimerInterval, false);
	TotalTime += PlayerStateTimerInterval;
}

void UOverheadWidget::ClearPlayerStateTimer(FTimerManager& TimerManager)
{
	PlayerStateTimerDelegate.Unbind();
	if(PlayerStateTimerHandle.IsValid())
	{
		TimerManager.ClearTimer(PlayerStateTimerHandle);
	}
}

void UOverheadWidget::NativeDestruct()
{
	RemoveFromParent();
	Super::NativeDestruct();
}
