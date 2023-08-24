// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"

#include "Blaster/BlasterComponents/LagCompensationComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void AHitScanWeapon::Fire(const TArray<FVector_NetQuantize>& HitTargets)
{
	Super::Fire(HitTargets);
	
	if(HitTargets.Num() == 0)
	{
		return;
	}

	const auto HitTarget = HitTargets[0];

	FTransform SocketTransform;
	FVector Start;
	GetSocketInformation(SocketTransform, Start);
	
	const FVector End = (Start + (HitTarget - Start) * 1.25f);
	
	if(const auto World = GetWorld())
	{
		FHitResult FireHit;
		PerformLineTrace(World, Start, End, FireHit);			
		PerformFireEffects(World, FireHit, SocketTransform);			
		if(PerformHit(World, FireHit))
		{
			ApplyDamage(FireHit.GetActor(), Damage, Start, HitTarget);
		}
	}
}

void AHitScanWeapon::PerformLineTrace(const UWorld* World, const FVector& Start, const FVector& End, FHitResult& FireHit)
{
	if(World)
	{
		World->LineTraceSingleByChannel(
			FireHit,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);
	}
}

void AHitScanWeapon::PerformFireEffects(UWorld* World, const FHitResult& FireHit, const FTransform& SocketTransform) const
{
	if(MuzzleFlash)
	{
		UGameplayStatics::SpawnEmitterAtLocation(World, MuzzleFlash, SocketTransform);
	}

	if(FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	FVector BeamEnd = FireHit.TraceEnd;
	if(FireHit.bBlockingHit)
	{
		BeamEnd = FireHit.ImpactPoint;
	}

	if(bShowDebugSpheres)
	{
		DrawDebugSphere(GetWorld(), BeamEnd, 16.f, 12, FColor::Orange, true);
	}

	if(BeamParticles)
	{
		if(const auto BeamParticlesComponent = UGameplayStatics::SpawnEmitterAtLocation(
			World,
			BeamParticles,
			SocketTransform
		))
		{
			BeamParticlesComponent->SetVectorParameter(FName("Target"), BeamEnd);
		}
	}
}

bool AHitScanWeapon::PerformHit(const UWorld* World, const FHitResult& FireHit) const
{
	if(!World)
	{
		return false;
	}

	bool bHitBlasterCharacter = false;
	if(FireHit.bBlockingHit)
	{
		if(Cast<ABlasterCharacter>(FireHit.GetActor()))
		{
			bHitBlasterCharacter = true;				
			PerformHitEffects(true, World, FireHit);
		}
		else
		{
			PerformHitEffects(false, World, FireHit);
		}
	}

	return bHitBlasterCharacter;
}

void AHitScanWeapon::ApplyDamage(AActor* HitActor, float InDamage, const FVector& TraceStart, const FVector& HitTarget)
{
	if(const auto OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if(const auto InstigatorController = Cast<AController>(OwnerPawn->Controller))
		{
			if(HasAuthority() && !bUseServerSideRewind)
			{
				UGameplayStatics::ApplyDamage(
					HitActor,
					InDamage,
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);						
			}
			
			if(!HasAuthority() && bUseServerSideRewind)
			{
				BlasterOwnerCharacter = !BlasterOwnerCharacter ? Cast<ABlasterCharacter>(OwnerPawn) : BlasterOwnerCharacter;
				BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterPlayerController>(InstigatorController) : BlasterOwnerController;
				if(BlasterOwnerController && BlasterOwnerCharacter && BlasterOwnerCharacter->GetLagCompensationComponent() && BlasterOwnerCharacter->IsLocallyControlled())
				{
					BlasterOwnerCharacter->GetLagCompensationComponent()->ServerScoreRequest(
						Cast<ABlasterCharacter>(HitActor),
						TraceStart,
						HitTarget,
						BlasterOwnerController->GetServerTime() - BlasterOwnerController->GetSingleTripTime(),
						this
					);
				}
			}
		}
	}
}

void AHitScanWeapon::PerformHitEffects(const bool bIsCharacterTarget, const UWorld* World, const FHitResult& FireHit) const
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