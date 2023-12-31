// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupHealth.h"

#include "Blaster/BlasterComponents/BuffComponent.h"
#include "Blaster/Character/BlasterCharacter.h"

APickupHealth::APickupHealth()
{
	bReplicates = true;
}

void APickupHealth::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                    UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComponent, OtherBodyIndex, bFromSweep, SweepResult);

	if(const auto BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		if(const auto Buff = BlasterCharacter->GetBuffComponent())
		{
			Buff->Heal(HealAmount, HealingTime);
		}
	}
	
	Destroy();
}