// Fill out your copyright notice in the Description page of Project Settings.


#include "PauseMenu.h"

#include "MultiplayerSessionsSubsystem.h"
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

	if(ReturnButton)
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
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UPauseMenu::ReturnButtonClicked()
{
	ReturnButton->SetIsEnabled(false);
	
	if(MultiplayerSessionsSubsystem != nullptr)
	{
		MultiplayerSessionsSubsystem->DestroySession();
	}
}

void UPauseMenu::OnDestroySession(bool bWasSuccessful)
{
	if(!bWasSuccessful && ReturnButton != nullptr)
	{
		ReturnButton->SetIsEnabled(true);
	}
	
	if(const UWorld* World = GetWorld())
	{
		if(const auto GameMode = World->GetAuthGameMode<AGameModeBase>())
		{
			GameMode->ReturnToMainMenuHost();
		}
		else
		{
			PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
			if(PlayerController)
			{
				PlayerController->ClientReturnToMainMenuWithTextReason(FText::FromString(TEXT("User has willingly vacated the premises")));
			}
		}
	}
}