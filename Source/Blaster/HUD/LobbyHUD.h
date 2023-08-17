// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LobbyHUD.generated.h"

class ULobbyOverlay;
/**
 * 
 */
UCLASS()
class BLASTER_API ALobbyHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	
	void AddLobbyOverlay();

	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> LobbyOverlayClass;

	UPROPERTY()
	ULobbyOverlay* LobbyOverlay;
};
