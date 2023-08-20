// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitScanWeapon.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const TArray<FVector_NetQuantize>& HitTargets) override;

private:
	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	USoundCue* ImpactSound;

	UPROPERTY(EditAnywhere)
	UParticleSystem* CharacterImpactParticles;

	UPROPERTY(EditAnywhere)
	USoundCue* CharacterImpactSound;

	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticles;

	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere)
	USoundCue* FireSound;

protected:
	
	UPROPERTY(EditAnywhere)
	float Damage = 20.0f;
	
	void PerformLineTrace(const UWorld* World, const FVector& Start, const FVector& End, FHitResult& FireHit);
	void ApplyDamage(AActor* HitActor, float InDamage);
	void PerformFireEffects(UWorld* World, const FHitResult& FireHit, const FTransform& SocketTransform) const;
	bool PerformHit(const UWorld* World, const FHitResult& FireHit) const;
	void PerformHitEffects(bool bIsCharacterTarget, const UWorld* World, const FHitResult& FireHit) const;
};
