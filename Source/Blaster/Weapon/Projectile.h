// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class AWeapon;
class UNiagaraComponent;
class UNiagaraSystem;
class UBoxComponent;
class UProjectileMovementComponent;
class UParticleSystem;
class UParticleSystemComponent;
class UPrimitiveComponent;
class USoundCue;

UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();

	virtual void Tick(float DeltaTime) override;

	virtual void Destroyed() override;

	float GetInitialSpeed() const;

	FORCEINLINE bool IsServerSideRewind() const { return bUseServerSideRewind; }
	FORCEINLINE void SetServerSideRewind(bool InServerSideRewind) { bUseServerSideRewind = InServerSideRewind; }

	FORCEINLINE void SetTraceStart(const FVector_NetQuantize& InStart) { TraceStart = InStart; }
	FORCEINLINE void SetInitialVelocity(const FVector_NetQuantize100& InInitialVelocity) { InitialVelocity = InInitialVelocity; }

	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE float GetHeadShotDamage() const { return HeadShotDamage; }
	FORCEINLINE void SetDamage(float InDamage) { Damage = InDamage; }
	FORCEINLINE void SetHeadShotDamage(float InDamage) { HeadShotDamage = InDamage; }

	FORCEINLINE void SetInnerDamageRadius(float InInnerDamageRadius) { InnerDamageRadius = InInnerDamageRadius; }
	FORCEINLINE void SetOuterDamageRadius(float InOuterDamageRadius) { OuterDamageRadius = InOuterDamageRadius; }
	FORCEINLINE void SetMinimumDamage(float InMinimumDamage) { MinimumDamage = InMinimumDamage; }
	
protected:
	virtual void BeginPlay() override;

	void StartDestroyTimer();
	void SpawnTrailSystem();
	void DestroyTimerFinished();
	void ApplyRadialDamage();
	
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayHitEffect(bool bHitEnemy);

	void PlayImpactEffects(AActor* OtherActorHit);

	
	UPROPERTY(EditAnywhere)
	UBoxComponent* CollisionBox;

	// Only set this for grenades and rockets
	UPROPERTY(EditAnywhere)
	float Damage = 20.0f;

	// Doesn't matter for grenades and rockets
	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 40.0f;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	USoundCue* ImpactSound;

	UPROPERTY(EditAnywhere)
	UParticleSystem* CharacterImpactParticles;

	UPROPERTY(EditAnywhere)
	USoundCue* CharacterImpactSound;
	
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* TrailSystem;	

	UPROPERTY()
	UNiagaraComponent* TrailSystemComponent;
	
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;
	
	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovementComponent;	

	/*
	 * Server Side Rewind
	 */

	UPROPERTY(EditAnywhere)
	bool bUseServerSideRewind = false;

	FVector_NetQuantize TraceStart;
	FVector_NetQuantize100 InitialVelocity;

	// The course uses an outside InitialSpeed float, but I'm choosing to rely on the projectile movement component
	// Initial Speed to set the values.
	
private:
	UPROPERTY(EditAnywhere)
	UParticleSystem* Tracer;

	UPROPERTY()
	UParticleSystemComponent* TracerComponent;

	int32 NumberOfConfirmedImpactEffects = 0;
	
	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.0;
	
	UPROPERTY(EditAnywhere)
	float InnerDamageRadius = 200.0f;

	UPROPERTY(EditAnywhere)
	float OuterDamageRadius = 500.0f;

	UPROPERTY(EditAnywhere)
	float MinimumDamage = 10.0f;
};
