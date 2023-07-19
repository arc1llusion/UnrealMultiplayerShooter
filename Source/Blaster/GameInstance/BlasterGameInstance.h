// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "BlasterGameInstance.generated.h"

class UUserWidget;
class UMenu;

UCLASS()
class BLASTER_API UBlasterGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UBlasterGameInstance();

	virtual void OnStart() override;
	
	UFUNCTION(BlueprintCallable)
	void CreateMenuWidget();
private:
	UPROPERTY()
	UMenu* OpeningWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Multiplayer Menu")
	TSubclassOf<UMenu> OpeningWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Multiplayer Menu")
	int32 NumberOfConnections = 4;
	
	UPROPERTY(EditDefaultsOnly, Category = "Multiplayer Menu")
	FString MatchType{FString(TEXT("FreeForAll"))};
	
	UPROPERTY(EditDefaultsOnly, Category = "Multiplayer Menu")
	FString PathToLobby{FString(TEXT("/Game/Maps/Lobby"))};

	void OnGameInstancePreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel);
};
