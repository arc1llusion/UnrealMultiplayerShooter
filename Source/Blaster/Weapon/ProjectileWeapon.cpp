// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Projectile.h"
#include "Engine/SkeletalMeshSocket.h"

void AProjectileWeapon::Fire(const TArray<FVector_NetQuantize>& HitTargets)
{
	Super::Fire(HitTargets);

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	UWorld* World = GetWorld();
	
	if(!ProjectileClass || !InstigatorPawn || !World)
	{
		return;
	}

	if(HitTargets.Num() == 0)
	{
		return;
	}

	const auto HitTarget = HitTargets[0];

	FTransform SocketTransform;
	FVector Start;
	GetSocketInformation(SocketTransform, Start);
	
	//From muzzle flash socket to hit location from TraceUnderCrossHairs
	const FVector ToTarget = HitTarget - Start;
	const FRotator TargetRotation = ToTarget.Rotation();
	
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = GetOwner();
	SpawnParameters.Instigator = InstigatorPawn;

	AProjectile* SpawnedProjectile = nullptr;

	if(bUseServerSideRewind)
	{
		if(InstigatorPawn->HasAuthority())
		{
			//On the server
			if(InstigatorPawn->IsLocallyControlled())
			{
				// Server, host, use replicated projectile
				// No need to use SSR on the server
				SpawnedProjectile = SpawnProjectile(World, SpawnParameters, Start, TargetRotation, true, false);
				SpawnedProjectile->SetDamage(Damage);
			}
			else
			{
				// Server, not locally controlled, spawn non-replicated projectile with server side rewind
				SpawnedProjectile = SpawnProjectile(World, SpawnParameters, Start, TargetRotation, false, true);
			}
		}
		else //Client, using server side rewind
		{			
			if(InstigatorPawn->IsLocallyControlled())
			{
				// Client, locally controlled, spawn non-replicated projectile and use ServerSideRewind
				SpawnedProjectile = SpawnProjectile(World, SpawnParameters, Start, TargetRotation, false, true);
				SpawnedProjectile->SetTraceStart(Start);
				SpawnedProjectile->SetInitialVelocity(SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->GetInitialSpeed());
				SpawnedProjectile->SetDamage(Damage);
			}
			else
			{
				// Client, not locally controlled, spawn non-replicated projectile and no ServerSideRewind
				SpawnedProjectile = SpawnProjectile(World, SpawnParameters, Start, TargetRotation, false, false);
			}
		}
	}
	else // Weapon not using ServerSideRewind
	{
		if(InstigatorPawn->HasAuthority())
		{
			SpawnedProjectile = SpawnProjectile(World, SpawnParameters, Start, TargetRotation, true, false);
			SpawnedProjectile->SetDamage(Damage);
		}
	}
}

AProjectile* AProjectileWeapon::SpawnProjectile(UWorld* World, const FActorSpawnParameters& InSpawnParams, const FVector& SpawnLocation,
                                                const FRotator& SpawnRotation, bool bInReplicated, bool bInUseServerSideRewind) const
{
	if(!World)
	{
		return nullptr;
	}

	const auto SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, InSpawnParams);
	SpawnedProjectile->SetReplicates(bInReplicated);
	SpawnedProjectile->SetServerSideRewind(bInUseServerSideRewind);

	return SpawnedProjectile;
}
