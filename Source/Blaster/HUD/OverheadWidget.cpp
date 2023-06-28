// Fill out your copyright notice in the Description page of Project Settings.


#include "OverheadWidget.h"

#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"

void UOverheadWidget::SetDisplayText(const FString& TextToDisplay)
{
	if(DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
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
		return; //Once the timer is set we don't need to do anything else
	}

	PlayerStateTimerDelegate.Unbind();
	if(PlayerStateTimerHandle.IsValid())
	{
		TimerManager.ClearTimer(PlayerStateTimerHandle);
	}

	if(PlayerState)
	{
		SetDisplayText(PlayerState->GetPlayerName());
	}
}

void UOverheadWidget::NativeDestruct()
{
	RemoveFromParent();
	Super::NativeDestruct();
}
