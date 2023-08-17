// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyHUD.h"

#include "LobbyOverlay.h"
#include "Blueprint/UserWidget.h"

void ALobbyHUD::BeginPlay()
{
	Super::BeginPlay();

	if(APlayerController* PlayerController = GetOwningPlayerController())
	{
		if(LobbyOverlayClass)
		{
			LobbyOverlay = CreateWidget<ULobbyOverlay>(PlayerController, LobbyOverlayClass);
			AddLobbyOverlay();
		}
	}
}

void ALobbyHUD::AddLobbyOverlay()
{
	if(LobbyOverlay)
	{
		LobbyOverlay->AddToViewport();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No Lobby Overlay Found"));
	}
}
