// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/GameInstance/BlasterGameInstance.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/GameMode/LobbyGameMode.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/HUD/AnnouncementWidget.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Blaster/HUD/LobbyHUD.h"
#include "Blaster/HUD/LobbyOverlay.h"
#include "Blaster/HUD/SniperScope.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetStringLibrary.h"
#include "Net/UnrealNetwork.h"

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, DesiredPawn)
	DOREPLIFETIME(ABlasterPlayerController, MatchState);
	DOREPLIFETIME(ABlasterPlayerController, bDisableGameplay);
	DOREPLIFETIME_CONDITION_NOTIFY(ABlasterPlayerController, PlayersReady, COND_None, REPNOTIFY_Always);
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());

	ServerCheckMatchState();	
 
	if (const ULocalPlayer* LocalBlasterPlayer = (GEngine && GetWorld()) ? GEngine->GetFirstGamePlayer(GetWorld()) : nullptr)
	{
		if (UEnhancedInputLocalPlayerSubsystem *Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalBlasterPlayer))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ABlasterPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SetHUDTime();
	CheckTimeSync(DeltaSeconds);
	CheckPing(DeltaSeconds);
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	if(const auto GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		WarmupTime = GameMode->GetWarmupTime();
		CooldownTime = GameMode->GetCooldownTime();
		MatchTime = GameMode->GetMatchTime();
		LevelStartingTime = GameMode->GetLevelStartingTime();
		MatchState = GameMode->GetMatchState();
		ClientJoinMidGame(MatchState, WarmupTime, CooldownTime, MatchTime, LevelStartingTime);		

		if(BlasterHUD && (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown))
		{
			BlasterHUD->AddAnnouncementOverlay();
		}
	}
}

void ABlasterPlayerController::ClientJoinMidGame_Implementation(FName InMatchState, float InWarmupTime, float InCooldownTime, float InMatchTime, float InStartingTime)
{
	WarmupTime = InWarmupTime;
	CooldownTime = InCooldownTime;
	MatchTime = InMatchTime;
	LevelStartingTime = InStartingTime;
	MatchState = InMatchState;
	OnMatchStateSet(MatchState);
	if(BlasterHUD && (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown))
	{
		BlasterHUD->AddAnnouncementOverlay();
	}
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if(const auto BlasterCharacter = Cast<ABlasterCharacter>(InPawn))
	{
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
		SetHUDShield(BlasterCharacter->GetShield(), BlasterCharacter->GetMaxShield());
		BlasterCharacter->UpdateHUDAmmo();
		BlasterCharacter->UpdateHUDReadyOverlay();

		if(BlasterCharacter->GetCombat())
		{
			SetHUDGrenades(BlasterCharacter->GetCombat()->GetGrenades());	
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("No combat component on character for grenades yet"));
		}	
	}
}

void ABlasterPlayerController::CheckTimeSync(float DeltaSeconds)
{
	TimeSyncRunningTime += DeltaSeconds;

	if(IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.0f;
	}
}

void ABlasterPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	if(const auto BlasterCharacter = Cast<ABlasterCharacter>(P))
	{
		BlasterCharacter->SetupOverheadOverlapEvents();
	}
}

void ABlasterPlayerController::SetDesiredPawn(const int32 InDesiredPawn)
{
	DesiredPawn = InDesiredPawn;	
	ServerSetPawn(DesiredPawn, true);
}

void ABlasterPlayerController::ServerSetPawn_Implementation(int32 InDesiredPawn, bool RequestRespawn)
{	
	if(const auto LobbyGameMode = Cast<ALobbyGameMode>(GetWorld()->GetAuthGameMode()))
	{
		DesiredPawn = InDesiredPawn;

		if(DesiredPawn >= LobbyGameMode->GetNumberOfPossibleCharacters())
		{
			DesiredPawn = 0;
		}
		else if(DesiredPawn < 0)
		{
			DesiredPawn = LobbyGameMode->GetNumberOfPossibleCharacters() - 1;
		}	

		SelectCharacter();

		if(RequestRespawn)
		{
			LobbyGameMode->RequestRespawn(Cast<ACharacter>(GetPawn()), this, true, GetPawn()->GetTransform());
		}
	}
}

void ABlasterPlayerController::SelectCharacter()
{
	if(const auto BlasterGameInstance = Cast<UBlasterGameInstance>(GetGameInstance()))
	{
		if(PlayerState)
		{
			const auto NetId = PlayerState->GetUniqueId();
			if(NetId.IsValid())
			{
				BlasterGameInstance->SelectCharacter(NetId->GetHexEncodedString(), DesiredPawn);
			}
		}
	}
}

void ABlasterPlayerController::ForwardDesiredPawn()
{
	ServerSetPawn(DesiredPawn + 1, true);
}

void ABlasterPlayerController::BackDesiredPawn()
{
	ServerSetPawn(DesiredPawn - 1, true);
}

void ABlasterPlayerController::SetReady()
{
	ServerSetReady();
}

void ABlasterPlayerController::ServerSetReady_Implementation()
{
	if(const auto LobbyGameMode = Cast<ALobbyGameMode>(GetWorld()->GetAuthGameMode()))
	{
		LobbyGameMode->SetPlayerReady(this);
	}
}

void ABlasterPlayerController::RegisterPlayersReady(TMap<ABlasterPlayerController*, bool> ReadyMap)
{
	PlayersReady.Empty();

	for(const auto& Item : ReadyMap)
	{
		if(!Item.Key->PlayerState)
		{
			continue;
		}
		
		FPlayersReady NewPlayersReady;
		NewPlayersReady.bIsReady = Item.Value;
		NewPlayersReady.PlayerName = Item.Key->PlayerState->GetPlayerName();

		PlayersReady.Add(NewPlayersReady);
	}

	for(const auto Item : PlayersReady)
	{
		const FString Ready = Item.bIsReady ? TEXT("READY") : TEXT("NOT READY");
		UE_LOG(LogTemp, Warning, TEXT("%s is %s"), *Item.PlayerName, *Ready);
	}

	SetHUDPlayersReady();
}

void ABlasterPlayerController::OnRep_PlayersReady()
{
	for(const auto Item : PlayersReady)
	{
		const FString Ready = Item.bIsReady ? TEXT("READY") : TEXT("NOT READY");
		UE_LOG(LogTemp, Warning, TEXT("%s: %s is %s"), *GetCharacter()->GetActorNameOrLabel(), *Item.PlayerName, *Ready);
	}

	SetHUDPlayersReady();
}

FString ABlasterPlayerController::GetPlayerId() const
{
	if(PlayerState)
	{
		const auto NetId = PlayerState->GetUniqueId();
		if(NetId.IsValid())
		{
			return NetId->GetHexEncodedString();
		}
	}

	return "";
}

void ABlasterPlayerController::SetHUDPlayersReady()
{
	LobbyHUD = !LobbyHUD ? Cast<ALobbyHUD>(GetHUD()) : LobbyHUD;

	bool bHudValid = LobbyHUD &&
		LobbyHUD->LobbyOverlay &&
		LobbyHUD->LobbyOverlay->PlayersReadyText;
	
	if(bHudValid)
	{
		TArray<FString> Lines;

		for(auto ReadyItem : PlayersReady)
		{
			FString Ready = ReadyItem.bIsReady ? TEXT("READY") : TEXT("NOT READY");
			Lines.Add(FString::Printf(TEXT("%s is %s"), *ReadyItem.PlayerName, *Ready));
		}
		
		const FString Output = UKismetStringLibrary::JoinStringArray(Lines, TEXT("\n"));
		LobbyHUD->LobbyOverlay->PlayersReadyText->SetText(FText::FromString(Output));
	}
	else
	{
		if(!LobbyHUD)
		{
			UE_LOG(LogTemp, Warning, TEXT("Lobby HUD Invalid"));
		}

		if(LobbyHUD && !LobbyHUD->LobbyOverlay)
		{
			UE_LOG(LogTemp, Warning, TEXT("Lobby HUD Overlay Invalid"));
		}

		if(LobbyHUD && LobbyHUD->LobbyOverlay && !LobbyHUD->LobbyOverlay->PlayersReadyText)
		{
			UE_LOG(LogTemp, Warning, TEXT("Lobby HUD Players Ready Text Invalid"));
		}		
	}
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HealthBar &&
		BlasterHUD->CharacterOverlay->HealthText;
	
	if(bHudValid)
	{
		
		const float HealthPercent = Health / MaxHealth;
		BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);

		const FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
}

void ABlasterPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->ShieldBar &&
		BlasterHUD->CharacterOverlay->ShieldText;
	
	if(bHudValid)
	{
		const float ShieldPercent = Shield / MaxShield;
		BlasterHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);

		const FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		BlasterHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
	}
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->ScoreAmount;

	if(bHudValid)
	{
		const FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
}

void ABlasterPlayerController::SetHUDDefeats(int32 InDefeats)
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->DefeatsAmount;

	if(bHudValid)
	{
		const FString DefeatsText = FString::Printf(TEXT("%d"), InDefeats);
		BlasterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
}

void ABlasterPlayerController::SetHUDDefeatsLog(const TArray<FString>& Logs)
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->DefeatsRelay;

	if(bHudValid)
	{		
		const FString Output = UKismetStringLibrary::JoinStringArray(Logs, TEXT("\n"));
		BlasterHUD->CharacterOverlay->DefeatsRelay->SetText(FText::FromString(Output));
	}
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 InAmmo)
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount;

	if(bHudValid)
	{
		const FString AmmoText = FString::Printf(TEXT("%d"), InAmmo);
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 InAmmo)
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount;

	if(bHudValid)
	{
		const FString AmmoText = FString::Printf(TEXT("%d"), InAmmo);
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void ABlasterPlayerController::SetHUDGrenades(int32 InGrenades)
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->GrenadesText;

	if(bHudValid)
	{
		const FString GrenadesText = FString::Printf(TEXT("%d"), InGrenades);
		BlasterHUD->CharacterOverlay->GrenadesText->SetText(FText::FromString(GrenadesText));
	}
}

void ABlasterPlayerController::SetHUDSniperScope(bool bIsAiming)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	const bool bHUDValid = BlasterHUD &&
		BlasterHUD->SniperScope &&
		BlasterHUD->SniperScope->ScopeZoomIn;
     
	if (!BlasterHUD->SniperScope)
	{
		BlasterHUD->AddSniperScope();
	}
     
	if (bHUDValid)
	{
		if (bIsAiming)
		{
			BlasterHUD->SniperScope->PlayAnimation(BlasterHUD->SniperScope->ScopeZoomIn);
		}
		else
		{
			BlasterHUD->SniperScope->PlayAnimation(
				BlasterHUD->SniperScope->ScopeZoomIn,
				0.f,
				1,
				EUMGSequencePlayMode::Reverse
			);
		}
	}
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->MatchCountdownText;

	if(bHudValid)
	{
		if(CountdownTime < 0.0f)
		{
			BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}
		
		const int32 Minutes = FMath::FloorToInt(CountdownTime / 60);
		const int32 Seconds = CountdownTime - (Minutes * 60);
		
		const FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHudValid = BlasterHUD &&
		BlasterHUD->AnnouncementOverlay &&
		BlasterHUD->AnnouncementOverlay->WarmupTime;

	if(bHudValid)
	{
		if(CountdownTime < 0.0f)
		{
			BlasterHUD->AnnouncementOverlay->WarmupTime->SetText(FText());
			return;
		}
		
		const int32 Minutes = FMath::FloorToInt(CountdownTime / 60);
		const int32 Seconds = CountdownTime - (Minutes * 60);
		
		const FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasterHUD->AnnouncementOverlay->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void ABlasterPlayerController::SetHUDTime()
{
	if (HasAuthority())
	{
		BlasterGameMode = !BlasterGameMode ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
		if (BlasterGameMode)
		{
			LevelStartingTime = BlasterGameMode->GetLevelStartingTime();
		}
	}
	
	const float TimeLeft = GetTimeLeft();
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	if(HasAuthority())
	{
		BlasterGameMode = !BlasterGameMode ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
		if(BlasterGameMode)
		{
			SecondsLeft = FMath::CeilToInt(BlasterGameMode->GetCountdownTime());
		}
	}

	if(Countdown != SecondsLeft)
	{
		if(MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		else if(MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}		
	}
	
	Countdown = SecondsLeft;	
}

float ABlasterPlayerController::GetTimeLeft() const
{
	float TimeLeft = 0.0f;
	if(MatchState == MatchState::WaitingToStart)
	{
		TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	}
	else if(MatchState == MatchState::InProgress)
	{
		TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	}
	else if(MatchState == MatchState::Cooldown)
	{
		TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	}

	return TimeLeft;
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	const float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest,
	float TimeServerReceivedClientRequest)
{
	const float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	SingleTripTime = 0.5f * RoundTripTime;
	const float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float ABlasterPlayerController::GetServerTime() const
{
	if(HasAuthority())
	{
		return GetWorld()->GetTimeSeconds();
	}

	return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if(IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ABlasterPlayerController::OnMatchStateSet(FName InMatchState)
{
	MatchState = InMatchState;

	if(MatchState == MatchState::InProgress)
	{		
		HandleMatchHasStarted();
	}
	else if(MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::OnRep_MatchState()
{
	if(MatchState == MatchState::InProgress)
	{		
		HandleMatchHasStarted();
	}
	else if(MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::HandleMatchHasStarted()
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if(BlasterHUD)
	{
		BlasterHUD->AddCharacterOverlay();
		
		if(BlasterHUD->AnnouncementOverlay)
		{
			BlasterHUD->AnnouncementOverlay->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ABlasterPlayerController::HandleCooldown()
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if(BlasterHUD)
	{
		if(BlasterHUD->CharacterOverlay)
		{
			BlasterHUD->CharacterOverlay->RemoveFromParent();
		}
		
		if( BlasterHUD->AnnouncementOverlay &&
			BlasterHUD->AnnouncementOverlay->AnnouncementText)
		{
			BlasterHUD->AnnouncementOverlay->SetVisibility(ESlateVisibility::Visible);

			const FString AnnouncementText("New Match Starts In:");
			BlasterHUD->AnnouncementOverlay->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			SetWinnerText();
		}
		else
		{
			BlasterHUD->AddAnnouncementOverlay();
		}
	}

	bDisableGameplay = true;
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =	ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{		
		Subsystem->ClearAllMappings();		
		Subsystem->AddMappingContext(LookOnlyMappingContext, 0);		
	}
}

void ABlasterPlayerController::CheckPing(float DeltaSeconds)
{
	HighPingRunningTime += DeltaSeconds;
	if(HighPingRunningTime > CheckPingFrequency)
	{
		PlayerState = !PlayerState ? GetPlayerState<APlayerState>() : PlayerState;
		if(PlayerState)
		{
			//GetPingInMilliseconds multiplies for 4 for us
			if (PlayerState->GetPingInMilliseconds() > HighPingThreshold)
			{
				StartHighPingWarning();
				PingAnimationRunningTime = 0.0f;
			}
		}

		HighPingRunningTime = 0.0f;
	}

	if(IsHighPingAnimationPlaying())
	{
		PingAnimationRunningTime += DeltaSeconds;
		if(PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
		}
	}
}

void ABlasterPlayerController::StartHighPingWarning()
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHudValid = BlasterHUD &&
	BlasterHUD->CharacterOverlay &&
	BlasterHUD->CharacterOverlay->HighPingImage &&
	BlasterHUD->CharacterOverlay->HighPingAnimation;

	if(bHudValid)
	{
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity(1.0f);
		BlasterHUD->CharacterOverlay->PlayAnimation(BlasterHUD->CharacterOverlay->HighPingAnimation, 0.0f, 5);
	}
}

void ABlasterPlayerController::StopHighPingWarning()
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHudValid = BlasterHUD &&
	BlasterHUD->CharacterOverlay &&
	BlasterHUD->CharacterOverlay->HighPingImage &&
	BlasterHUD->CharacterOverlay->HighPingAnimation;

	if(bHudValid)
	{
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity(0.0f);

		if(BlasterHUD->CharacterOverlay->IsAnimationPlaying(BlasterHUD->CharacterOverlay->HighPingAnimation))
		{
			BlasterHUD->CharacterOverlay->StopAnimation(BlasterHUD->CharacterOverlay->HighPingAnimation);
		}
	}
}

bool ABlasterPlayerController::IsHighPingAnimationPlaying() const
{
	return BlasterHUD &&
		   BlasterHUD->CharacterOverlay &&
		   BlasterHUD->CharacterOverlay->HighPingAnimation &&
		   BlasterHUD->CharacterOverlay->IsAnimationPlaying(BlasterHUD->CharacterOverlay->HighPingAnimation);
}

void ABlasterPlayerController::SetWinnerText()
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if(BlasterHUD &&
	   BlasterHUD->AnnouncementOverlay &&
	   BlasterHUD->AnnouncementOverlay->InfoText)
	{
		ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
		const ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if(BlasterGameState && BlasterPlayerState)
		{				
			TArray<ABlasterPlayerState*> TopPlayers;
			BlasterGameState->GetTopScoringPlayers(TopPlayers);

			FString InfoTextString = TEXT("");

			if(TopPlayers.Num() == 0)
			{
				InfoTextString = FString(TEXT("There is no winner"));
			}
			else if(TopPlayers.Num() == 1 && TopPlayers[0] == BlasterPlayerState)
			{
				InfoTextString = FString(TEXT("You are the winner!"));
			}
			else if(TopPlayers.Num() == 1)
			{
				InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
			}
			else if(TopPlayers.Num() > 1)
			{
				InfoTextString = FString("Players tied for the win: \n");

				for(const auto TiedPlayer : TopPlayers)
				{
					InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
				}
			}

			BlasterHUD->AnnouncementOverlay->InfoText->SetText(FText::FromString(InfoTextString));
		}			
	}
}
