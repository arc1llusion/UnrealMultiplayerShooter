// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"

#include "Blaster/BlasterComponents/LagCompensationComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"

void AShotgun::Fire(const TArray<FVector_NetQuantize>& HitTargets)
{
	AWeapon::Fire(HitTargets);

	UWorld* World = GetWorld();

	if(!World)
	{
		return;
	}
	
	FTransform SocketTransform;
	FVector Start;
	GetSocketInformation(SocketTransform, Start);

	//Maps hit character to the number of times that character was hit by shotgun scatter shots
	TMap<ABlasterCharacter*, uint32> Hits;
	for(const auto Hit : HitTargets)
	{
		FHitResult FireHit;
		PerformLineTrace(World, Start, Hit, FireHit);
		PerformFireEffects(World, FireHit, SocketTransform);
		
		if(PerformHit(World, FireHit))
		{
			if(ABlasterCharacter* HitActor = Cast<ABlasterCharacter>(FireHit.GetActor()))
			{
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
	}

	if(const auto OwnerPawn = Cast<APawn>(GetOwner()))
	{
		const bool bCauseAuthorityDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled(); 
		if(HasAuthority() && bCauseAuthorityDamage)
		{
			ApplyDamageToAllHitActors(Hits);				
		}
		if(!HasAuthority() && bUseServerSideRewind)
		{	
			TArray<ABlasterCharacter*> HitCharacters;
			Hits.GetKeys(HitCharacters);
		
			BlasterOwnerCharacter = !BlasterOwnerCharacter ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
			BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller) : BlasterOwnerController;
			if(BlasterOwnerController && BlasterOwnerCharacter && BlasterOwnerCharacter->GetLagCompensationComponent() && BlasterOwnerCharacter->IsLocallyControlled())
			{
				UE_LOG(LogTemp, Warning, TEXT("Performing shotgun server score request"));
				BlasterOwnerCharacter->GetLagCompensationComponent()->ShotgunServerScoreRequest(
					HitCharacters,
					Start,
					HitTargets,
					BlasterOwnerController->GetServerTime() - BlasterOwnerController->GetSingleTripTime()
				);
			}
		}
	}
}

void AShotgun::ApplyDamageToAllHitActors(const TMap<ABlasterCharacter*, uint32>& HitActors)
{
	for(auto& HitCheck : HitActors)
	{
		ApplyDamage(HitCheck.Key, HitCheck.Value * Damage, FVector::ZeroVector, FVector::ZeroVector);
	}
}

