// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);	

	FTransform SocketTransform;
	FVector Start;
	GetSocketInformation(SocketTransform, Start);
	
	const FVector End = bUseScatter ? TraceEndWithScatter(Start, HitTarget) : (Start + (HitTarget - Start) * 1.25f);
	
	if(const auto World = GetWorld())
	{
		FHitResult FireHit;
		PerformLineTrace(World, Start, End, FireHit);			
		PerformFireEffects(World, FireHit, SocketTransform);			
		if(PerformHit(World, FireHit))
		{
			ApplyDamage(FireHit.GetActor(), Damage);
		}
	}
}

void AHitScanWeapon::GetSocketInformation(FTransform& OutSocketTransform, FVector& OutStart) const
{
	OutSocketTransform = FTransform::Identity;
	OutStart = FVector::ZeroVector;
	
	if(const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(MuzzleSocketFlashName))
	{
		OutSocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		OutStart = OutSocketTransform.GetLocation();		
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

void AHitScanWeapon::ApplyDamage(AActor* HitActor, float InDamage)
{
	if(const auto OwnerPawn = Cast<APawn>(GetOwner()))
	{
		const auto InstigatorController = Cast<AController>(OwnerPawn->Controller);
			
		if(HasAuthority() && InstigatorController)
		{
			UGameplayStatics::ApplyDamage(
				HitActor,
				InDamage,
				InstigatorController,
				this,
				UDamageType::StaticClass()
			);						
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

FVector AHitScanWeapon::TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget) const
{
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	const FVector RandomVector = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.0f, SphereRadius);
	const FVector EndLocation = SphereCenter + RandomVector;
	const FVector ToEndLocation = EndLocation - TraceStart;

	if(bShowDebugSpheres)
	{
		DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
		DrawDebugSphere(GetWorld(), EndLocation, 4.0f, 12, FColor::Orange, true);
		DrawDebugLine(GetWorld(), TraceStart, FVector(TraceStart + ToEndLocation * TRACE_LENGTH / ToEndLocation.Size()), FColor::Cyan, true);
	}
	
	return FVector(TraceStart + ToEndLocation * TRACE_LENGTH / ToEndLocation.Size());
}
