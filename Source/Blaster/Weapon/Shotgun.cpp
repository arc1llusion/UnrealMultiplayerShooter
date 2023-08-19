// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"

// void AShotgun::Fire(const FVector& HitTarget)
// {
// 	AWeapon::Fire(HitTarget);
//
// 	UWorld* World = GetWorld();
//
// 	if(!World)
// 	{
// 		return;
// 	}
// 	
// 	FTransform SocketTransform ;
// 	FVector Start;
// 	GetSocketInformation(SocketTransform, Start);
//
// 	TMap<AActor*, uint32> Hits;
// 	GetHitActors(World, SocketTransform, Start, HitTarget, Hits);
// 	ApplyDamageToAllHitActors(Hits);	
// }

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
	AWeapon::Fire(FVector::ZeroVector);

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

void AShotgun::GetHitActors(UWorld* World, FTransform SocketTransform, FVector Start, const FVector& HitTarget, TMap<AActor*, uint32>& OutHits)
{
	OutHits.Empty();
	
	for(uint32 i = 0; i < NumberOfShots; ++i)
	{
		FVector End = TraceEndWithScatter(HitTarget);

		FHitResult FireHit;
		PerformLineTrace(World, Start, End, FireHit);
		PerformFireEffects(World, FireHit, SocketTransform);	

		if(PerformHit(World, FireHit))
		{
			AActor* HitActor = FireHit.GetActor();
			if(!OutHits.Contains(HitActor))
			{
				OutHits.Emplace(HitActor, 1);
			}
			else
			{
				OutHits[HitActor]++;
			}
		}
	}
}

void AShotgun::ApplyDamageToAllHitActors(const TMap<AActor*, uint32>& HitActors)
{
	for(auto& HitCheck : HitActors)
	{
		ApplyDamage(HitCheck.Key, HitCheck.Value * Damage);
	}
}

void AShotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& OutHitTargets)
{
	FTransform SocketTransform;
	FVector TraceStart;
	GetSocketInformation(SocketTransform, TraceStart);
	
	for(uint32 Shot = 0; Shot < NumberOfShots; ++Shot)
	{
		OutHitTargets.Add(TraceEndWithScatter(HitTarget, TraceStart));
	}
}

