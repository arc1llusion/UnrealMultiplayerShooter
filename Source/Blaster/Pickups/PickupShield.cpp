// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupShield.h"

#include "Blaster/BlasterComponents/BuffComponent.h"
#include "Blaster/Character/BlasterCharacter.h"

APickupShield::APickupShield()
{
	bReplicates = true;
}

void APickupShield::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
									UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComponent, OtherBodyIndex, bFromSweep, SweepResult);

	if(const auto BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		if(const auto Buff = BlasterCharacter->GetBuffComponent())
		{
			Buff->ReplenishShield(ShieldReplenishAmount, ShieldReplenishTime);
		}
	}
	
	Destroy();
}
