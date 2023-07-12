// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterHUD.h"

const FHUDPackage FHUDPackage::NullPackage{nullptr, nullptr, nullptr, nullptr, nullptr, 0.0f, FLinearColor::White};

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();
	
	FVector2D ViewportSize;
	if(GEngine)
	{
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