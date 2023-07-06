// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Projectile.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	
	if(!ProjectileClass || !InstigatorPawn)
	{
		return;
	}

	if(const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName(TEXT("MuzzleFlash"))))
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());

		//From muzzle flash socket to hit location from TraceUnderCrosshairs
		const FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		const FRotator TargetRotation = ToTarget.Rotation();
		
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = GetOwner();
		SpawnParameters.Instigator = InstigatorPawn;
		
		if(UWorld* World = GetWorld())
		{
			World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParameters);
		}		
	}
}
