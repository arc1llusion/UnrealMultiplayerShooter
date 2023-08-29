// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HitScanWeapon.h"
#include "Shotgun.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AShotgun : public AHitScanWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const TArray<FVector_NetQuantize>& HitTargets) override;

private:

	void ApplyDamageToAllHitActors(const TMap<ABlasterCharacter*, float>& HitActors);
};
