// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupSpeed.h"

#include "Blaster/BlasterComponents/BuffComponent.h"
#include "Blaster/Character/BlasterCharacter.h"

void APickupSpeed::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComponent, OtherBodyIndex, bFromSweep, SweepResult);

	if(const auto BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		if(const auto Buff = BlasterCharacter->GetBuffComponent())
		{
			Buff->BuffSpeed(BaseSpeedBuff, CrouchSpeedBuff, AimSpeedBuff, Duration);
		}
	}
	
	Destroy();
}
