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
	virtual void Fire(const FVector& HitTarget) override;


private:
	UPROPERTY(EditAnywhere)
	FName MuzzleSocketFlashName{"MuzzleFlash"};

	UPROPERTY(EditAnywhere)
	float Damage = 20.0f;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	USoundCue* ImpactSound;

	UPROPERTY(EditAnywhere)
	UParticleSystem* CharacterImpactParticles;

	UPROPERTY(EditAnywhere)
	USoundCue* CharacterImpactSound;

	void PerformLineTrace(const FVector& HitTarget, FHitResult& FireHit, const AController* InstigatorController) const;
	void PerformHit(const UWorld* World, const FHitResult& FireHit, AController* InstigatorController);
	void PerformHitEffects(bool bIsCharacterTarget, const UWorld* World, const FHitResult& FireHit) const;
};
