// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);
	
	FHitResult FireHit;
	PerformLineTrace(HitTarget, FireHit);
}

void AHitScanWeapon::PerformLineTrace(const FVector& HitTarget, FHitResult& FireHit)
{
	if(const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(MuzzleSocketFlashName))
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector Start = SocketTransform.GetLocation();
		const FVector End = Start + (HitTarget - Start) * 1.25f;
		
		if(const auto World = GetWorld())
		{
			World->LineTraceSingleByChannel(
				FireHit,
				Start,
				End,
				ECollisionChannel::ECC_Visibility
			);			

			PerformFireEffects(World, SocketTransform);
			
			PerformHit(World, FireHit, SocketTransform);
		}
	}
}

void AHitScanWeapon::PerformFireEffects(UWorld* World, const FTransform& SocketTransform) const
{
	if(MuzzleFlash)
	{
		UGameplayStatics::SpawnEmitterAtLocation(World, MuzzleFlash, SocketTransform);
	}

	if(FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}
}

void AHitScanWeapon::PerformHit(UWorld* World, const FHitResult& FireHit, const FTransform& SocketTransform)
{
	if(!World)
	{
		return;
	}

	FVector BeamEnd = FireHit.TraceEnd;
	if(FireHit.bBlockingHit)
	{
		BeamEnd = FireHit.ImpactPoint;

		if(const auto BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor()))
		{
			ApplyDamage(BlasterCharacter);					
			PerformHitEffects(true, World, FireHit);
		}
		else
		{
			PerformHitEffects(false, World, FireHit);
		}
	}

	if(BeamParticles)
	{
		BeamParticlesComponent = UGameplayStatics::SpawnEmitterAtLocation(
			World,
			BeamParticles,
			SocketTransform
		);

		if(BeamParticlesComponent)
		{
			BeamParticlesComponent->SetVectorParameter(FName("Target"), BeamEnd);
		}
	}
}

void AHitScanWeapon::ApplyDamage(ABlasterCharacter* const BlasterCharacter)
{
	if(const auto OwnerPawn = Cast<APawn>(GetOwner()))
	{
		const auto InstigatorController = Cast<AController>(OwnerPawn->Controller);
			
		if(HasAuthority() && InstigatorController)
		{
			UGameplayStatics::ApplyDamage(
				BlasterCharacter,
				Damage,
				InstigatorController,
				this,
				UDamageType::StaticClass()
			);						
		}
	}
}

void AHitScanWeapon::PerformHitEffects(bool bIsCharacterTarget, const UWorld* World, const FHitResult& FireHit) const
{
	if(!World)
	{
		return;
	}
	
	if(bIsCharacterTarget)
	{
		if(CharacterImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				World,
				CharacterImpactParticles,
				FireHit.ImpactPoint,
				FireHit.ImpactNormal.Rotation()
			);
		}

		if(CharacterImpactSound)
		{
			UGameplayStatics::PlaySoundAtLocation(World, CharacterImpactSound, GetActorLocation());
		}
	}
	else
	{
		if(ImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				World,
				ImpactParticles,
				FireHit.ImpactPoint,
				FireHit.ImpactNormal.Rotation()
			);
		}

		if(ImpactSound)
		{
			UGameplayStatics::PlaySoundAtLocation(World, ImpactSound, GetActorLocation());
		}
	}
}
