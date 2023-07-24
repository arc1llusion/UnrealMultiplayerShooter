// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/GameInstance/BlasterGameInstance.h"
#include "Blaster/GameMode/LobbyGameMode.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/KismetStringLibrary.h"
#include "Net/UnrealNetwork.h"

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, DesiredPawn)
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	BlasterHUD = Cast<ABlasterHUD>(GetHUD());

	if(GetPawn())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Begin Play"), *GetPawn()->GetActorNameOrLabel());
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
		const FString DefeatsText = FString::Printf(TEXT("%d"), InAmmo);
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(DefeatsText));
	}
}
