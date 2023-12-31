// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

class UEliminationAnnouncement;
class USniperScope;
class UAnnouncementWidget;
class UTexture2D;
class UCharacterOverlay;

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UTexture2D* CrosshairsCenter = nullptr;
	UPROPERTY()
	UTexture2D* CrosshairsLeft = nullptr;
	UPROPERTY()
	UTexture2D* CrosshairsRight = nullptr;
	UPROPERTY()
	UTexture2D* CrosshairsTop = nullptr;
	UPROPERTY()
	UTexture2D* CrosshairsBottom = nullptr;

	float CrosshairSpread;

	FLinearColor CrosshairsColor;

	BLASTER_API static const FHUDPackage NullPackage;
};

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
	void AddCharacterOverlay();
	void AddAnnouncementOverlay();
	void AddEliminationAnnouncementOverlay(const FString& AttackerName, const FString& VictimName);
	void AddSniperScope();

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<UUserWidget> CharacterOverlayClass;

	UPROPERTY()
	UCharacterOverlay* CharacterOverlay;

	UPROPERTY(EditAnywhere, Category = "Announcements")
	TSubclassOf<UUserWidget> AnnouncementClass;

	UPROPERTY()
	UAnnouncementWidget* AnnouncementOverlay;

	UPROPERTY(EditAnywhere, Category = "Sniper Scope")
	TSubclassOf<UUserWidget> SniperScopeClass;

	UPROPERTY()
	USniperScope* SniperScope;

protected:
	virtual void BeginPlay() override;;
	
private:
	FHUDPackage HUDPackage;

	UPROPERTY()
	APlayerController* OwningPlayer;

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.0f;

	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor);

	UPROPERTY(EditAnywhere, Category = "Announcements")
	TSubclassOf<UUserWidget> EliminationAnnouncementClass;

	UPROPERTY()
	UEliminationAnnouncement* EliminationAnnouncement;

	UPROPERTY(EditAnywhere)
	float EliminationAnnouncementTime = 3.0f;

	UFUNCTION()
	void EliminationAnnouncementTimerFinished(UEliminationAnnouncement* MessageToRemove);

	UPROPERTY()
	TArray<UEliminationAnnouncement*> EliminationMessages;
public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }

	
};
