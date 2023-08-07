// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "Blueprint/UserWidget.h"
#include "PickupWidget.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class BLASTER_API UPickupWidget : public UUserWidget
{
	GENERATED_BODY()


public:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* PickupText;

	void SetWeaponText(const EWeaponType WeaponType) const;
};
