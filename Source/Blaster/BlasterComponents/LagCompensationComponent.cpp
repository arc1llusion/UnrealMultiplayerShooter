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

	if(FrameHistory.Num() <= 1)
	{
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		
		FrameHistory.AddHead(ThisFrame);
	}
	else
	{
		float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;

		while(HistoryLength > MaxRecordTime)
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}

		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		
		FrameHistory.AddHead(ThisFrame);
	}
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

FFramePackage ULagCompensationComponent::InterpolationBetweenFrames(const FFramePackage& OlderFrame,
	const FFramePackage& YoungerFrame, const float HitTime) const
{
	const float Distance = YoungerFrame.Time - OlderFrame.Time;
	const float InterpolationFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.0f, 1.0f);

	FFramePackage InterpolationFramePackage;
	InterpolationFramePackage.Time = HitTime;

	for(auto& YoungerPair : YoungerFrame.HitBoxInfo)
	{
		const FName& BoneName = YoungerPair.Key;

		if(!OlderFrame.HitBoxInfo.Contains(BoneName))
		{
			continue;
		}

		const FCapsuleInformation& YoungerCapsule = YoungerPair.Value;
		const FCapsuleInformation& OlderCapsule = OlderFrame.HitBoxInfo[BoneName];
		
		FCapsuleInformation InterpolationCapsuleInformation;

		InterpolationCapsuleInformation.Location = FMath::Lerp(OlderCapsule.Location, YoungerCapsule.Location, InterpolationFraction);
		InterpolationCapsuleInformation.Rotation = FMath::Lerp(OlderCapsule.Rotation, YoungerCapsule.Rotation, InterpolationFraction);
		InterpolationCapsuleInformation.HalfHeight = YoungerCapsule.HalfHeight;
		InterpolationCapsuleInformation.Radius = YoungerCapsule.Radius;

		InterpolationFramePackage.HitBoxInfo.Add(BoneName, InterpolationCapsuleInformation);
	}

	return InterpolationFramePackage;
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
			false,
			4.0f
		);
	}
}

void ULagCompensationComponent::ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize& HitLocation, float HitTime)
{
	bool bReturn =
		!HitCharacter ||
		!HitCharacter->GetLagCompensationComponent() ||
		!HitCharacter->GetLagCompensationComponent()->FrameHistory.GetHead() ||
		!HitCharacter->GetLagCompensationComponent()->FrameHistory.GetTail(); 

	if(bReturn)
	{
		return;
	}

	// Frame package that we check to verify a hit
	FFramePackage FrameToCheck;
	bool bShouldInterpolate = true;

	//Frame history of the hit character
	const auto& History = HitCharacter->GetLagCompensationComponent()->FrameHistory;
	const float OldestHistoryTime = History.GetTail()->GetValue().Time;
	const float NewestHistoryTime = History.GetHead()->GetValue().Time;
	if(OldestHistoryTime > HitTime)
	{
		// too far back - too much lag to do server side rewind
		return;
	}

	if(OldestHistoryTime == HitTime)
	{
		//Unlikely, but found an exact match at the tail end of the history, so we don't need to interpolate
		FrameToCheck = History.GetTail()->GetValue();
		bShouldInterpolate = false;
	}

	if(NewestHistoryTime <= HitTime)
	{
		// Unlikely, but we were given a HitTime that is greater than or equal to the newest time in the history
		// In this case, just use the newest history frame package
		FrameToCheck = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	}

	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = History.GetHead();

	//Is older still younger than HitTime?
	while(Older->GetValue().Time > HitTime)
	{
		// March back until: OlderTime < HitTime < YoungerTime
		if(Older->GetNextNode() == nullptr)
		{
			break;
		}
		Older = Older->GetNextNode();

		if(Older->GetValue().Time > HitTime)
		{
			Younger = Older;
		}
	}

	if(Older->GetValue().Time == HitTime)
	{
		FrameToCheck = Older->GetValue();
		bShouldInterpolate = false;
	}

	if(bShouldInterpolate)
	{
		//Interpolate between younger and older
	}
}
