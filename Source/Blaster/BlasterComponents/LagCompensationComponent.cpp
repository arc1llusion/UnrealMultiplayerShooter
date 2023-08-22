// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Components/CapsuleComponent.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = !Character ? Cast<ABlasterCharacter>(GetOwner()) : Character;

	if(!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("LagCompensation SaveFramePackage no Character"));
		return;
	}

	Package.Time = GetWorld()->GetTimeSeconds();

	for(const auto & CapsulePair : Character->HitCollisionCapsules)
	{
		FCapsuleInformation CapsuleInformation;
		CapsuleInformation.Location = CapsulePair.Value->GetComponentLocation();
		CapsuleInformation.Rotation = CapsulePair.Value->GetComponentRotation();
		CapsuleInformation.Radius = CapsulePair.Value->GetUnscaledCapsuleRadius();
		CapsuleInformation.HalfHeight = CapsulePair.Value->GetUnscaledCapsuleHalfHeight();
		
		Package.HitBoxInfo.Add(CapsulePair.Key, CapsuleInformation);
	}
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor& Color) const
{
	for(const auto & CapsulePair : Package.HitBoxInfo)
	{
		DrawDebugCapsule(
			GetWorld(),
			CapsulePair.Value.Location,
			CapsulePair.Value.HalfHeight,
			CapsulePair.Value.Radius,
			FQuat(CapsulePair.Value.Rotation),
			Color,
			true
		);
	}
}
