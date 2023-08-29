// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterHUD.h"

#include "AnnouncementWidget.h"
#include "CharacterOverlay.h"
#include "EliminationAnnouncement.h"
#include "SniperScope.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"

const FHUDPackage FHUDPackage::NullPackage{nullptr, nullptr, nullptr, nullptr, nullptr, 0.0f, FLinearColor::White};

void ABlasterHUD::BeginPlay()
{
	Super::BeginPlay();

	if(APlayerController* PlayerController = GetOwningPlayerController())
	{
		if(CharacterOverlayClass)
		{
			CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
		}
	}
}

void ABlasterHUD::AddCharacterOverlay()
{
	if(CharacterOverlay)
	{
		CharacterOverlay->AddToViewport();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No Character Overlay Found"));
	}
}

void ABlasterHUD::AddAnnouncementOverlay()
{
	if(AnnouncementOverlay)
	{
		return;
	}

	if(APlayerController* PlayerController = GetOwningPlayerController())
	{
		if(AnnouncementClass)
		{
			AnnouncementOverlay = CreateWidget<UAnnouncementWidget>(PlayerController, AnnouncementClass);
			
			if(AnnouncementOverlay)
			{
				AnnouncementOverlay->AddToViewport();
			}
		}
	}
}

void ABlasterHUD::AddEliminationAnnouncementOverlay(const FString& AttackerName, const FString& VictimName)
{
	OwningPlayer = OwningPlayer == nullptr ? GetOwningPlayerController() : OwningPlayer;
	if(OwningPlayer && EliminationAnnouncementClass)
	{
		EliminationAnnouncement = CreateWidget<UEliminationAnnouncement>(OwningPlayer, EliminationAnnouncementClass);

		EliminationAnnouncement->SetEliminationAnnouncementText(AttackerName, VictimName);
		EliminationAnnouncement->AddToViewport();

		for(const UEliminationAnnouncement* Message : EliminationMessages)
		{
			if(Message && Message->AnnouncementBox)
			{
				if(UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Message->AnnouncementBox))
				{
					const FVector2D Position = CanvasSlot->GetPosition();
					const FVector2D NewPosition{Position.X, Position.Y - CanvasSlot->GetSize().Y};

					CanvasSlot->SetPosition(NewPosition);
				}
			}
		}

		EliminationMessages.Add(EliminationAnnouncement);

		FTimerHandle EliminationMessageTimer;
		FTimerDelegate EliminationMessageDelegate;

		EliminationMessageDelegate.BindUFunction(this, GET_FUNCTION_NAME_STRING_CHECKED(ABlasterHUD, EliminationAnnouncementTimerFinished), EliminationAnnouncement);
		GetWorldTimerManager().SetTimer(EliminationMessageTimer, EliminationMessageDelegate, EliminationAnnouncementTime, false);
	}
}

void ABlasterHUD::EliminationAnnouncementTimerFinished(UEliminationAnnouncement* MessageToRemove)
{
	if(MessageToRemove)
	{
		MessageToRemove->RemoveFromParent();
		EliminationMessages.Remove(MessageToRemove);
	}
}

void ABlasterHUD::AddSniperScope()
{
	if(AnnouncementOverlay)
	{
		return;
	}

	if(APlayerController* PlayerController = GetOwningPlayerController())
	{
		if(SniperScopeClass)
		{
			SniperScope = CreateWidget<USniperScope>(PlayerController, SniperScopeClass);
			
			if(SniperScope)
			{
				SniperScope->AddToViewport();
			}
		}
	}
}

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();

	if(GEngine)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D ViewportCenter{ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f};

		float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;

		if(HUDPackage.CrosshairsCenter)
		{
			DrawCrosshair(HUDPackage.CrosshairsCenter, ViewportCenter, FVector2D::Zero(), HUDPackage.CrosshairsColor);
		}

		if(HUDPackage.CrosshairsLeft)
		{
			const FVector2D Spread(-SpreadScaled, 0.0f);
			DrawCrosshair(HUDPackage.CrosshairsLeft, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}

		if(HUDPackage.CrosshairsRight)
		{
			const FVector2D Spread(SpreadScaled, 0.0f);
			DrawCrosshair(HUDPackage.CrosshairsRight, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}

		if(HUDPackage.CrosshairsTop)
		{
			const FVector2D Spread(0.0f, -SpreadScaled);
			DrawCrosshair(HUDPackage.CrosshairsTop, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}

		if(HUDPackage.CrosshairsBottom)
		{
			const FVector2D Spread(0.0f, SpreadScaled);
			DrawCrosshair(HUDPackage.CrosshairsBottom, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}
	}
}

void ABlasterHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor)
{
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();
	const FVector2D TextureDrawPoint {ViewportCenter.X - (TextureWidth / 2.0f) + Spread.X, ViewportCenter.Y - (TextureHeight / 2.0f) + Spread.Y};

	DrawTexture
	(
		Texture,
		TextureDrawPoint.X,
		TextureDrawPoint.Y,
		TextureWidth,
		TextureHeight,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
		CrosshairColor
	);
}
