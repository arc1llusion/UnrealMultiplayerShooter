// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupWidget.h"

#include "Components/TextBlock.h"

void UPickupWidget::SetWeaponText(const EWeaponType WeaponType) const
{
	FString Output = TEXT("Assault Rifle");

	switch(WeaponType)
	{
		case EWeaponType::EWT_Pistol:
			Output = "Pistol";
			break;
		case EWeaponType::EWT_Shotgun:
			Output = "Shotgun";
			break;
		case EWeaponType::EWT_GrenadeLauncher:
			Output = "Grenade Launcher";
			break;
		case EWeaponType::EWT_RocketLauncher:
			Output = "Rocket Launcher";
			break;
		case EWeaponType::EWT_SniperRifle:
			Output = "Sniper Rifle";
			break;
		case EWeaponType::EWT_SubMachineGun:
			Output = "SMG";
			break;
		case EWeaponType::EWT_AssaultRifle:
			Output = "Assault Rifle";
			break;
		case EWeaponType::EWT_Flag:
			Output = "Flag";
			break;
		default:
			Output = "I dunno";
			break;
	}

	PickupText->SetText(FText::FromString(FString::Printf(TEXT("E - Pickup: %s"), *Output)));
}
