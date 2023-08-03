// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	const auto OwnerPawn = Cast<APawn>(GetOwner());
	if(!OwnerPawn)
	{
		return;
	}

	const auto InstigatorController = Cast<AController>(OwnerPawn->Controller);
	
	FHitResult FireHit;
	PerformLineTrace(HitTarget, FireHit, InstigatorController);
	PerformHit(GetWorld(), FireHit, InstigatorController);
}

void AHitScanWeapon::PerformLineTrace(const FVector& HitTarget, FHitResult& FireHit, const AController* InstigatorController) const
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(MuzzleSocketFlashName);
	if(MuzzleFlashSocket && InstigatorController)
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
		}		
	}
}

void AHitScanWeapon::PerformHit(const UWorld* World, const FHitResult& FireHit, AController* InstigatorController)
{
	if(FireHit.bBlockingHit)
	{
		if(const auto BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor()))
		{
			if(HasAuthority())
			{
				UGameplayStatics::ApplyDamage(
					BlasterCharacter,
					Damage,
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);						
			}
					
			PerformHitEffects(true, World, FireHit);
		}
		else
		{
			PerformHitEffects(false, World, FireHit);
		}
	}
}

void AHitScanWeapon::PerformHitEffects(bool bIsCharacterTarget, const UWorld* World, const FHitResult& FireHit) const
{
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
