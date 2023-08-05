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

	/*
	 * Trace End With Scatter
	 */
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bShowDebugSpheres = false;

protected:
	UPROPERTY(EditAnywhere)
	FName MuzzleSocketFlashName{"MuzzleFlash"};
	
	UPROPERTY(EditAnywhere)
	float Damage = 20.0f;
	
	FVector TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget) const;

	void GetSocketInformation(FTransform& OutSocketTransform, FVector& OutStart) const;
	void PerformLineTrace(const UWorld* World, const FVector& Start, const FVector& End, FHitResult& FireHit);
	void ApplyDamage(AActor* HitActor, float InDamage);
	void PerformFireEffects(UWorld* World, const FHitResult& FireHit, const FTransform& SocketTransform) const;
	bool PerformHit(const UWorld* World, const FHitResult& FireHit) const;
	void PerformHitEffects(bool bIsCharacterTarget, const UWorld* World, const FHitResult& FireHit) const;
};
