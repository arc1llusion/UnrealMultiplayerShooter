// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"

void AShotgun::Fire(const TArray<FVector_NetQuantize>& HitTargets)
{
	AWeapon::Fire(HitTargets);

	UWorld* World = GetWorld();

	if(!World)
	{
		return;
	}
	
	FTransform SocketTransform ;
	FVector Start;
	GetSocketInformation(SocketTransform, Start);

	//Maps hit character to the number of times that character was hit by shotgun scatter shots
	TMap<AActor*, uint32> Hits;
	for(const auto Hit : HitTargets)
	{
		FHitResult FireHit;
		PerformLineTrace(World, Start, Hit, FireHit);
		PerformFireEffects(World, FireHit, SocketTransform);
		
		if(PerformHit(World, FireHit))
		{
			AActor* HitActor = FireHit.GetActor();
			if(!Hits.Contains(HitActor))
			{
				Hits.Emplace(HitActor, 1);
			}
			else
			{
				Hits[HitActor]++;
			}
		}
	}

	ApplyDamageToAllHitActors(Hits);
}

void AShotgun::ApplyDamageToAllHitActors(const TMap<AActor*, uint32>& HitActors)
{
	for(auto& HitCheck : HitActors)
	{
		ApplyDamage(HitCheck.Key, HitCheck.Value * Damage);
	}
}

