// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"

#include "Blaster/BlasterComponents/LagCompensationComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
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
	if(const ABlasterCharacter* OwnerCharacter = Cast<ABlasterCharacter>(GetOwner()))
	{
		ABlasterPlayerController* OwnerController = Cast<ABlasterPlayerController>(OwnerCharacter->Controller);

		if(OwnerCharacter)
		{
			if(OwnerCharacter->HasAuthority() && !bUseServerSideRewind)
			{
				bool bIsHeadShot = false;

				if(const auto HitBlasterCharacter = Cast<ABlasterCharacter>(Hit.GetActor()))
				{
					bIsHeadShot = Hit.BoneName == HitBlasterCharacter->GetHeadHitBoxName();
				}
				
				UGameplayStatics::ApplyDamage(OtherActor, bIsHeadShot ? HeadShotDamage : Damage, OwnerController, this, UDamageType::StaticClass());
				Super::OnHit(HitComponent, OtherActor, OtherComponent, NormalImpulse, Hit);
				return;
			}

			ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(OtherActor);
			if(bUseServerSideRewind && OwnerCharacter->GetLagCompensationComponent() && OwnerCharacter->IsLocallyControlled() && HitCharacter)
			{
				OwnerCharacter->GetLagCompensationComponent()->ProjectileServerScoreRequest(
					HitCharacter,
					TraceStart,
					InitialVelocity,
					OwnerController->GetServerTime() - OwnerController->GetSingleTripTime()
				);
			}
		}
	}	
	
	Super::OnHit(HitComponent, OtherActor, OtherComponent, NormalImpulse, Hit);
}
