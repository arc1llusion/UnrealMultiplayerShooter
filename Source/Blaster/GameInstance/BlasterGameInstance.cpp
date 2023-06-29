// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameInstance.h"
#include "Engine/GameInstance.h"
#include "MultiplayerSessions/Public/Menu.h"


UBlasterGameInstance::UBlasterGameInstance()
{
}

void UBlasterGameInstance::OnStart()
{
	Super::OnStart();

	CreateMenuWidget();
	NotifyPreClientTravelDelegates.AddUObject(this, &UBlasterGameInstance::OnGameInstancePreClientTravel);
}

void UBlasterGameInstance::CreateMenuWidget()
{
	OpeningWidget = Cast<UMenu>(CreateWidget(this, OpeningWidgetClass, TEXT("OpeningWidget")));

	if(OpeningWidget)
	{
		OpeningWidget->MenuSetup(NumberOfConnections, MatchType, PathToLobby);
	}
}

void UBlasterGameInstance::OnGameInstancePreClientTravel(const FString& PendingURL, ETravelType TravelType,
	bool bIsSeamlessTravel) const
{	
	if(OpeningWidget && bIsSeamlessTravel && PendingURL.Contains(TEXT("Lobby")))
	{		
		OpeningWidget->MenuTearDown();
	}
}
