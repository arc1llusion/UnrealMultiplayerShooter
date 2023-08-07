// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupWidget.h"

#include "Components/TextBlock.h"

void UPickupWidget::SetWeaponText(const EWeaponType WeaponType) const
{
	FText EnumText;
	UEnum::GetDisplayValueAsText(WeaponType, EnumText);

	const FString EnumString = EnumText.ToString();
	const FString Output = FString::Printf(TEXT("E - Pickup: %s"), *EnumString);

	PickupText->SetText(FText::FromString(Output));
}
