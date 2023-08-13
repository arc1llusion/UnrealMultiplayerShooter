// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "PickupJump.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API APickupJump : public APickup
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
	float JumpZVelocity = 4000.0f;

	UPROPERTY(EditAnywhere)
	float Duration = 30.0f;
};
