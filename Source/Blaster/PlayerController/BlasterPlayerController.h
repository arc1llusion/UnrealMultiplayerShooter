// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

class ABlasterPlayerState;
class ABlasterHUD;
class AGameMode;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 InDefeats);
	void SetHUDDefeatsLog(const TArray<FString>& Logs);
	void SetHUDWeaponAmmo(int32 InAmmo);
	void SetHUDCarriedAmmo(int32 InAmmo);
	void SetHUDMatchCountdown(float CountdownTime);
	
	
	virtual void OnPossess(APawn* InPawn) override;

	virtual void AcknowledgePossession(APawn* P) override;

	void ForwardDesiredPawn();
	void BackDesiredPawn();
	FORCEINLINE int32 GetDesiredPawn() const { return DesiredPawn; }
	void SetDesiredPawn(const int32 InDesiredPawn);	

protected:
	virtual void BeginPlay() override;

	void SetHUDTime();

	UFUNCTION(Reliable, Server)
	virtual void ServerSetPawn(int32 InDesiredPawn, bool RequestRespawn);

	virtual void SelectCharacter();	
	
private:
	UPROPERTY()
	ABlasterHUD* BlasterHUD;

	float MatchTime = 120.0f;
	uint32 Countdown = 0;

	UPROPERTY(VisibleAnywhere, Replicated)
	int32 DesiredPawn = 0;
};
