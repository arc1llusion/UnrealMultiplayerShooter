// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

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
	UNiagaraSystem* TrailSystem;	

	UPROPERTY()
	UNiagaraComponent* TrailSystemComponent;
	
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;
	
	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovementComponent;
private:
	UPROPERTY(EditAnywhere)
	UParticleSystem* Tracer;

	UPROPERTY()
	UParticleSystemComponent* TracerComponent;

	int32 NumberOfConfirmedImpactEffects = 0;
	
	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.0;
	
	UPROPERTY(EditAnywhere, Category = "Radial Damage Falloff")
	float InnerDamageRadius = 200.0f;

	UPROPERTY(EditAnywhere, Category = "Radial Damage Falloff")
	float OuterDamageRadius = 500.0f;

	UPROPERTY(EditAnywhere, Category = "Radial Damage Falloff")
	float MinimumDamage = 10.0f;
};
