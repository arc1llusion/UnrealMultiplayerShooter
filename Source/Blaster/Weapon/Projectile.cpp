// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"

#include "NiagaraFunctionLibrary.h"
#include "Blaster/Blaster.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);

	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if(Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
												Tracer,
												CollisionBox,
												NAME_None,
												GetActorLocation(),
												GetActorRotation(),
												EAttachLocation::KeepWorldPosition);
	}

	if(HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}
}

void AProjectile::StartDestroyTimer()
{
	GetWorldTimerManager().SetTimer(DestroyTimer, this, &AProjectile::DestroyTimerFinished, DestroyTime);
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
                        FVector NormalImpulse, const FHitResult& Hit)
{
	PlayImpactEffects(OtherActor);
}

void AProjectile::ApplyRadialDamage()
{
	if(const APawn* FiringPawn = GetInstigator(); FiringPawn && HasAuthority())
	{
		if(AController* FiringController = FiringPawn->GetController())
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,//World Context Object
				Damage, //Base Damage
				MinimumDamage, // Minimum Damage
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
}

void AProjectile::PlayImpactEffects(AActor* OtherActorHit)
{
	if(Cast<ABlasterCharacter>(OtherActorHit))
	{
		MulticastPlayHitEffect(true);
	}
	else
	{
		MulticastPlayHitEffect(false);
	}
}

void AProjectile::SpawnTrailSystem()
{	
	if(TrailSystem)
	{
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
}

void AProjectile::DestroyTimerFinished()
{
	Destroy();
}

void AProjectile::Destroyed()
{
	Super::Destroyed();
}

void AProjectile::MulticastPlayHitEffect_Implementation(bool bHitEnemy)
{
	if(bHitEnemy)
	{
		if(CharacterImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), CharacterImpactParticles, GetActorTransform());
		}

		if(CharacterImpactSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, CharacterImpactSound, GetActorLocation());
		}
	}
	else
	{
		if(ImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
		}

		if(ImpactSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
		}
	}
	
	Destroy();
}
