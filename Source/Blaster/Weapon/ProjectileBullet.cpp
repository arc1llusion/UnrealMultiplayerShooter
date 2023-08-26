// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"

#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"


AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
}

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();

	// FPredictProjectilePathParams PathParams;
	// PathParams.bTraceWithChannel = true;
	// PathParams.bTraceWithCollision = true;
	// PathParams.DrawDebugTime = 5.0f;
	// PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	// PathParams.LaunchVelocity = GetActorForwardVector() * ProjectileMovementComponent->InitialSpeed;
	// PathParams.MaxSimTime = 4.0f;
	// PathParams.ProjectileRadius = 5.0f;
	// PathParams.SimFrequency = 30.0f;
	// PathParams.StartLocation = GetActorLocation();
	// PathParams.TraceChannel = ECollisionChannel::ECC_Visibility;
	// PathParams.ActorsToIgnore.Add(this);	
	//
	// FPredictProjectilePathResult PathResult;
	// UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
                              UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());

	if(OwnerCharacter)
	{
		AController* OwnerController = OwnerCharacter->Controller;

		if(OwnerCharacter)
		{
			UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
		}
	}	
	
	Super::OnHit(HitComponent, OtherActor, OtherComponent, NormalImpulse, Hit);
}
