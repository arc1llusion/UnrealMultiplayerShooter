// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "PickupAmmo.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API APickupAmmo : public APickup
{
	GENERATED_BODY()

protected:
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	) override;
	
private:
	UPROPERTY(EditAnywhere)
	int32 AmmoAmount = 30;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType = EWeaponType::EWT_AssaultRifle;
};
