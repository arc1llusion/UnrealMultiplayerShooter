// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/BlasterTypes/FPlayersReady.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "BlasterPlayerController.generated.h"

class UPauseMenu;
class ALobbyHUD;
class ABlasterPlayerState;
class ABlasterHUD;
class UInputMappingContext;
class UInputAction;
class ABlasterGameMode;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);

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

	/*
	 * Lobby HUD
	 */

	void SetHUDPlayersReady();
	
	/*
	 * Blaster HUD
	 */ 
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 InDefeats);
	void SetHUDDefeatsLog(const TArray<FString>& Logs);
	void SetHUDWeaponAmmo(int32 InAmmo);
	void SetHUDCarriedAmmo(int32 InAmmo);
	void SetHUDGrenades(int32 InGrenades);
	void SetHUDSniperScope(bool bIsAiming);
	void SetHUDMatchCountdown(float CountdownTime);
	void SetHUDAnnouncementCountdown(float CountdownTime);	
	void SetHUDTime();
	
	void BroadcastElimination(APlayerState* Attacker, APlayerState* Victim);
	void BroadcastFallElimination(APlayerState* Victim);

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

	FORCEINLINE bool IsGameplayDisabled() const { return bDisableGameplay; }
	
	void SetReady();
	void RegisterPlayersReady(TMap<ABlasterPlayerController*, bool> ReadyMap);

	FString GetPlayerId() const;

	FORCEINLINE float GetSingleTripTime() const { return SingleTripTime; }

	FHighPingDelegate HighPingDelegate;

protected:
	virtual void BeginPlay() override;	

	/*
	 * Input
	 */	

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext *DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext *LookOnlyMappingContext;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction *PauseInputAsset;
	
	UPROPERTY(Replicated)
	bool bDisableGameplay;
	
	/*
	 * Pause
	 */

	virtual void SetupInputComponent() override;	

	void PauseAction(const FInputActionValue &Value);

	UPROPERTY(EditAnywhere, Category = "HUD")
	TSubclassOf<UUserWidget> PauseWidget;

	UPROPERTY()
	UPauseMenu* PauseMenu;

	bool bPauseOpen = false;
	
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
	void ClientJoinMidGame(FName InMatchState, float InWarmupTime, float InCooldownTime, float InMatchTime, float InStartingTime);

	void CheckPing(float DeltaSeconds);
	void StartHighPingWarning();
	void StopHighPingWarning();	
	bool IsHighPingAnimationPlaying() const;

	UFUNCTION(Reliable, Server)
	virtual void ServerSetPawn(int32 InDesiredPawn, bool RequestRespawn);
	virtual void SelectCharacter();

	UFUNCTION(Reliable, Server)
	virtual void ServerSetReady();

	UFUNCTION(Client, Reliable)
	void ClientEliminationAnnouncement(APlayerState* Attacker, APlayerState* Victim);
	
private:
	UPROPERTY()
	ABlasterHUD* BlasterHUD;

	UPROPERTY()
	ALobbyHUD* LobbyHUD;

	UPROPERTY()
	ABlasterGameMode* BlasterGameMode;

	float MatchTime = 0.0f;
	float WarmupTime = 0.0f;
	float CooldownTime = 0.0f;
	float LevelStartingTime = 0.0f;
	uint32 Countdown = 0;

	UPROPERTY(VisibleAnywhere, Replicated)
	int32 DesiredPawn = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	float GetTimeLeft() const;

	void SetWinnerText();

	UPROPERTY(ReplicatedUsing = OnRep_PlayersReady)
	TArray<FPlayersReady> PlayersReady;

	UFUNCTION()
	void OnRep_PlayersReady();

	float HighPingRunningTime = 0.0f;

	float PingAnimationRunningTime = 0.0f;

	UPROPERTY(EditAnywhere, Category = "High Ping Warning")
	float HighPingDuration = 5.0f;

	UPROPERTY(EditAnywhere, Category = "High Ping Warning")
	float CheckPingFrequency = 20.0f;

	UPROPERTY(EditAnywhere, Category = "High Ping Warning")
	float HighPingThreshold = 50.0f;

	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bHighPing);

	float SingleTripTime = 0.0f;
};
