// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "PickupShield.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API APickupShield : public APickup
{
	GENERATED_BODY()

public:
	APickupShield();

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
	float ShieldReplenishAmount = 100.0f;

	UPROPERTY(EditAnywhere)
	float ShieldReplenishTime = 5.0f;
};
