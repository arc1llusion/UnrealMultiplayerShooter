// Fill out your copyright notice in the Description page of Project Settings.


#include "PauseMenu.h"

#include "MultiplayerSessionsSubsystem.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/GameInstance/BlasterGameInstance.h"
#include "Components/Button.h"
#include "GameFramework/GameModeBase.h"

bool UPauseMenu::Initialize()
{
	if(!Super::Initialize())
	{
		return false;
	}

	return true;
}

void UPauseMenu::MenuSetup()
{
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = true;

	if(const UWorld* World = GetWorld())
	{
		PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
		if(PlayerController)
		{
			FInputModeGameAndUI InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}	

	if(ReturnButton && !ReturnButton->OnClicked.IsBound())
	{
		ReturnButton->OnClicked.AddDynamic(this, &UPauseMenu::ReturnButtonClicked);
	}

	if(const UGameInstance* GameInstance = GetGameInstance())
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		if(MultiplayerSessionsSubsystem)
		{
			MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &UPauseMenu::OnDestroySession);
		}
	}
}

void UPauseMenu::MenuTearDown()
{
	RemoveFromParent();
	SetVisibility(ESlateVisibility::Hidden);
	bIsFocusable = false;

	if(const UWorld* World = GetWorld())
	{
		PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
		if(PlayerController)
		{
			const FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}

	if(ReturnButton && ReturnButton->OnClicked.IsBound())
	{
		ReturnButton->OnClicked.RemoveDynamic(this, &UPauseMenu::ReturnButtonClicked);
	}

	if(MultiplayerSessionsSubsystem && MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.IsBound())
	{
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.RemoveDynamic(this, &UPauseMenu::OnDestroySession);
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UPauseMenu::ReturnButtonClicked()
{
	ReturnButton->SetIsEnabled(false);

	if(const auto World = GetWorld())
	{
		if(const APlayerController* FirstPlayerController = World->GetFirstPlayerController())
		{
			if(const auto BlasterCharacter = Cast<ABlasterCharacter>(FirstPlayerController->GetPawn()))
			{
				BlasterCharacter->OnLeftGame.AddDynamic(this, &UPauseMenu::OnPlayerLeftGame);
				BlasterCharacter->ServerLeaveGame();
			}
			else
			{
				ReturnButton->SetIsEnabled(true);
			}
		}
	}
}

void UPauseMenu::OnDestroySession(bool bWasSuccessful)
{
	if(!bWasSuccessful && ReturnButton != nullptr)
	{
		ReturnButton->SetIsEnabled(true);
		//return;
	}
	
	if(const UWorld* World = GetWorld())
	{		
		if(const auto GameMode = World->GetAuthGameMode<AGameModeBase>())
		{
			UE_LOG(LogTemp, Warning, TEXT("On Destroy Session Server Reached"));
			GameMode->ReturnToMainMenuHost();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("On Destroy Session Client Reached"));
			PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
			if(PlayerController)
			{
				PlayerController->ClientReturnToMainMenuWithTextReason(FText::FromString(TEXT("User has willingly vacated the premises")));
			}
		}
	}
}

void UPauseMenu::OnPlayerLeftGame()
{
	if(MultiplayerSessionsSubsystem != nullptr)
	{
		MultiplayerSessionsSubsystem->DestroySession();
	}
}
