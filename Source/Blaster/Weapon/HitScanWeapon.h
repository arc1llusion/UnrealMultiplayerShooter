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

	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticles;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* BeamParticlesComponent;

	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere)
	USoundCue* FireSound;

	void PerformLineTrace(const FVector& HitTarget, FHitResult& FireHit);
	void ApplyDamage(ABlasterCharacter* BlasterCharacter);
	void PerformFireEffects(UWorld* World, const FTransform& SocketTransform) const;
	void PerformHit(UWorld* World, const FHitResult& FireHit, const FTransform& SocketTransform);
	void PerformHitEffects(bool bIsCharacterTarget, const UWorld* World, const FHitResult& FireHit) const;
};
