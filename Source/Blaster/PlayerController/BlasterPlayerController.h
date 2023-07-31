// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

class ABlasterPlayerState;
class ABlasterHUD;
class AGameMode;
class UInputMappingContext;
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
	void SetHUDAnnouncementCountdown(float CountdownTime);

	//Synced with server world clock
	virtual float GetServerTime() const;
	//Sync with server clock as soon as possible
	virtual void ReceivedPlayer() override;

	void OnMatchStateSet(FName InMatchState);
	
	virtual void OnPossess(APawn* InPawn) override;
	virtual void AcknowledgePossession(APawn* P) override;

	void ForwardDesiredPawn();
	void BackDesiredPawn();
	FORCEINLINE int32 GetDesiredPawn() const { return DesiredPawn; }
	void SetDesiredPawn(const int32 InDesiredPawn);	

protected:
	virtual void BeginPlay() override;

	void SetHUDTime();

	/*
	 * Sync time between client and server
	 */

	// Requests current server time, passing in the clients time when the request was sent
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	// Reports the current server time to the client in response to ServerRequestServerTime
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	//Difference between client and server time
	float ClientServerDelta = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Server Time Synchronization")
	float TimeSyncFrequency = 5.0f;

	UPROPERTY(VisibleAnywhere, Category = "Server Time Synchronization")
	float TimeSyncRunningTime = 0.0f;

	void CheckTimeSync(float DeltaSeconds);

	void HandleMatchHasStarted();
	void HandleCooldown();

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
	void ClientJoinMidGame(FName InMatchState, float InWarmupTime, float InMatchTime, float InStartingTime);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext *DefaultMappingContext;

	UFUNCTION(Reliable, Server)
	virtual void ServerSetPawn(int32 InDesiredPawn, bool RequestRespawn);
	virtual void SelectCharacter();	
	
private:
	UPROPERTY()
	ABlasterHUD* BlasterHUD;

	float MatchTime = 0.0f;
	float WarmupTime = 0.0f;
	float LevelStartingTime = 0.0f;
	uint32 Countdown = 0;

	UPROPERTY(VisibleAnywhere, Replicated)
	int32 DesiredPawn = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	float GetTimeLeft() const;

};
