// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"

#include "Kismet/GameplayStatics.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
                              UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if(const APawn* FiringPawn = GetInstigator())
	{
		if(AController* FiringController = FiringPawn->GetController())
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,//World Context Object
				Damage, //Base Damage
				Damage * 0.5f,// Minimum Damage
				GetActorLocation(), //Origin
				InnerDamageRadius,
				OuterDamageRadius,
				1.0f, //Falloff Damage Curve
				UDamageType::StaticClass(),
				TArray<AActor*>(),
				this, //Damage Causer
				FiringController // Instigator Controller
			);
		}
	}
	
	Super::OnHit(HitComponent, OtherActor, OtherComponent, NormalImpulse, Hit);
}
