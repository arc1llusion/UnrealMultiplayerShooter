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
#include "Blaster/PlayerState/BlasterPlayerState.h"
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
			else
			{
				if(GetPawn())
				{
					UE_LOG(LogTemp, Warning, TEXT("%s: No Unique Id"), *GetPawn()->GetActorNameOrLabel());
				}
			}
		}
		else
		{
			if(GetPawn())
			{
				UE_LOG(LogTemp, Warning, TEXT("%s: No Player State"), *GetPawn()->GetActorNameOrLabel());
			}
		}
	}
	else
	{
		if(GetPawn())
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: No Game Instance"), *GetPawn()->GetActorNameOrLabel());
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
	const float CurrentServerTime = TimeServerReceivedClientRequest + (RoundTripTime * 0.5f);

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
