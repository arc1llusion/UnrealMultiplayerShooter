// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "PickupSpeed.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API APickupSpeed : public APickup
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
	float BaseSpeedBuff = 1600.0f;

	UPROPERTY(EditAnywhere)
	float CrouchSpeedBuff = 850.0f;

	UPROPERTY(EditAnywhere)
	float Duration = 30.0f;
};
