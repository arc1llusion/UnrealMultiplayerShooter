// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileRocket.generated.h"


class UNiagaraComponent;
class UNiagaraSystem;

/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileRocket();

protected:
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void BeginPlay() override;

	void ApplyRadialDamage();
	void DisableFxOnHit();

	void DestroyTimerFinished();
	
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* TrailSystem;	

	UPROPERTY()
	UNiagaraComponent* TrailSystemComponent;

	UPROPERTY(EditAnywhere)
	USoundCue* ProjectileLoop;

	UPROPERTY()
	UAudioComponent* ProjectileLoopComponent;

	UPROPERTY(EditAnywhere)
	USoundAttenuation* LoopingSoundAttenuation;
private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* RocketMesh;
	
	UPROPERTY(EditAnywhere, Category = "Radial Damage Falloff")
	float InnerDamageRadius = 200.0f;

	UPROPERTY(EditAnywhere, Category = "Radial Damage Falloff")
	float OuterDamageRadius = 500.0f;

	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.0;
};
