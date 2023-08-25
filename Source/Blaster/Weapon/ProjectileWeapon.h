// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "ProjectileWeapon.generated.h"


class AProjectile;


/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const TArray<FVector_NetQuantize>& HitTargets) override;
	
private:
	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile> ProjectileClass;

	AProjectile* SpawnProjectile(
		UWorld* World,
		const FActorSpawnParameters& InSpawnParams,
		const FVector& SpawnLocation,
		const FRotator& SpawnRotation,
		bool bInReplicated,
		bool bInUseServerSideRewind
	) const;
};
