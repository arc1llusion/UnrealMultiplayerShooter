// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupAmmo.h"

#include "Blaster/Character/BlasterCharacter.h"

void APickupAmmo::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                  UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComponent, OtherBodyIndex, bFromSweep, SweepResult);

	if(const auto BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		if(const auto Combat = BlasterCharacter->GetCombat())
		{
			Combat->PickupAmmo(WeaponType, AmmoAmount);
		}
	}

	Destroy();
}
