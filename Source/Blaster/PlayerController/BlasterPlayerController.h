// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

class ABlasterPlayerState;
class ABlasterHUD;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 InDefeats);
	void SetHUDDefeatsLog(const TArray<FString>& Logs);
	
	virtual void OnPossess(APawn* InPawn) override;

	virtual void AcknowledgePossession(APawn* P) override;
	
protected:
	virtual void BeginPlay() override;
	
private:
	UPROPERTY()
	ABlasterHUD* BlasterHUD;
};
