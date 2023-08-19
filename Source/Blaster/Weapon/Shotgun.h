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
	//virtual void Fire(const FVector& HitTarget) override;

	virtual void FireShotgun(const TArray<FVector_NetQuantize>& HitTargets);

	virtual void ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& OutHitTargets);

private:
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	uint32 NumberOfShots = 10;

	void GetHitActors(UWorld* World, FTransform SocketTransform, FVector Start, const FVector& HitTarget, TMap<AActor*, uint32>& OutHits);
	void ApplyDamageToAllHitActors(const TMap<AActor*, uint32>& HitActors);
};
