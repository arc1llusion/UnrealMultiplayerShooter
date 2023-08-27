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

	// if(const UWorld* World = GetWorld())
	// {
	// 	if(GEngine)
	// 	{
	// 		const FString MapName = FString::Printf(TEXT("Map Name: %s"), *World->GetMapName());
	// 		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, MapName);
	// 	}
	// 	
	// 	if(World->GetMapName().Contains("Main"))
	// 	{				
	// 		CreateMenuWidget();
	// 		NotifyPreClientTravelDelegates.AddUObject(this, &UBlasterGameInstance::OnGameInstancePreClientTravel);
	// 	}
	// }
}

void UBlasterGameInstance::CreateMenuWidget()
{
	OpeningWidget = Cast<UMenu>(CreateWidget(this, OpeningWidgetClass, TEXT("OpeningWidget")));

	if(OpeningWidget)
	{
		OpeningWidget->MenuSetup(NumberOfConnections, MatchType, PathToLobby);
	}
}

void UBlasterGameInstance::SelectCharacter(const FString& PlayerId, int32 InDesiredPawn)
{
	UE_LOG(LogTemp, Warning, TEXT("Selecting Character %s %d"), *PlayerId, InDesiredPawn);
	
	if(PlayerCharacterSelections.Contains(PlayerId))
	{
		PlayerCharacterSelections[PlayerId] = InDesiredPawn;
	}
	else
	{
		PlayerCharacterSelections.Add(PlayerId, InDesiredPawn);
	}
}

int32 UBlasterGameInstance::GetSelectedCharacter(const FString& PlayerId) const
{
	//UE_LOG(LogTemp, Warning, TEXT("Checking Player Character Selection %s"), *PlayerId);
	if(PlayerCharacterSelections.Contains(PlayerId))
	{
		UE_LOG(LogTemp, Warning, TEXT("Found: %d"), PlayerCharacterSelections[PlayerId]);
		return PlayerCharacterSelections[PlayerId];
	}

	//UE_LOG(LogTemp, Warning, TEXT("Not Found"));
	return DefaultCharacterPawnType;
}

void UBlasterGameInstance::OnGameInstancePreClientTravel(const FString& PendingURL, ETravelType TravelType,
                                                         bool bIsSeamlessTravel)
{
	if(PendingURL.Contains("Main"))
	{		
		CreateMenuWidget();
	}
	else
	{		
		if(OpeningWidget)
		{		
			OpeningWidget->MenuTearDown();
			OpeningWidget = nullptr;
		}
	}
}
