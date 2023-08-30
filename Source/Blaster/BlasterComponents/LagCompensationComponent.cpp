// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"

#include "Blaster/Blaster.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Weapon/Projectile.h"
#include "Blaster/Weapon/Weapon.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

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

	SaveFrameHistory();
}

void ULagCompensationComponent::SaveFrameHistory()
{
	if(!Character || !Character->HasAuthority())
	{
		return;
	}

	//If no frame history exists, simply save the current frame as we have no point of reference
	if(FrameHistory.Num() <= 1)
	{
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		
		FrameHistory.AddHead(ThisFrame);
	}
	else
	{
		//The total time between the oldest (Tail) and youngest (Head) of the linked list
		float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;

		//Remove nodes until the history length is less than the maximum allotted amount of time
		while(HistoryLength > MaxRecordTime)
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}

		//Then save the current frame
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
	Package.Character = Character;

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

void ULagCompensationComponent::ServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter,
                                                                  const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	if(!Character)
	{
		return;
	}
	
	const FServerSideRewindResult Confirm = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);

	//If server side rewind was a success, apply damage
	if(HitCharacter && Character->GetEquippedWeapon() && Confirm.bIsHit)
	{
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			Confirm.bIsHeadShot ? Character->GetEquippedWeapon()->GetHeadShotDamage() : Character->GetEquippedWeapon()->GetDamage(),
			Character->Controller,
			Character->GetEquippedWeapon(),
			UDamageType::StaticClass());
	}
}


void ULagCompensationComponent::ProjectileServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, const float HitTime, AProjectile* DamageCauser)
{
	if(!Character)
	{
		return;
	}
	
	const FServerSideRewindResult Confirm = ProjectileServerSideRewind(HitCharacter, TraceStart, InitialVelocity, HitTime);

	//If server side rewind was a success, apply damage
	if(HitCharacter && DamageCauser && Confirm.bIsHit)
	{
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			Confirm.bIsHeadShot ? DamageCauser->GetHeadShotDamage() : DamageCauser->GetDamage(),
			Character->Controller,
			DamageCauser,
			UDamageType::StaticClass());
	}
}

void ULagCompensationComponent::ShotgunServerScoreRequest_Implementation(
	const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart,
	const TArray<FVector_NetQuantize>& HitLocations, const float HitTime)
{
	if(!Character)
	{
		return;
	}
	
	FShotgunServerSideRewindResult Confirm = ShotgunServerSideRewind(HitCharacters, TraceStart, HitLocations, HitTime);

	for(auto& HitCharacter : HitCharacters)
	{
		if(!HitCharacter || !Character->GetEquippedWeapon())
			continue;

		float TotalDamage = 0.0f;
		
		if(Confirm.HeadShots.Contains(HitCharacter))
		{
			const float HeadShotDamage = Confirm.HeadShots[HitCharacter] * Character->GetEquippedWeapon()->GetHeadShotDamage();
			TotalDamage += HeadShotDamage;
		}
		if(Confirm.BodyShots.Contains(HitCharacter))
		{
			const float BodyShotDamage = Confirm.BodyShots[HitCharacter] * Character->GetEquippedWeapon()->GetDamage();
			TotalDamage += BodyShotDamage;
		}

		UE_LOG(LogTemp, Warning, TEXT("Total Damage: %f"), TotalDamage);
		
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			TotalDamage,
			Character->Controller,
			Character->GetEquippedWeapon(),
			UDamageType::StaticClass()
		);
	}
}


FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart,
                                                                    const FVector_NetQuantize& HitLocation, float HitTime)
{
	const FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
	return ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitLocation);
}


FServerSideRewindResult ULagCompensationComponent::ProjectileServerSideRewind(ABlasterCharacter* HitCharacter,
                                                                              const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	const FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
	return ProjectileConfirmHit(FrameToCheck, HitCharacter, TraceStart, InitialVelocity, HitTime);
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(
	const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart,
	const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	TArray<FFramePackage> FramesToCheck;
	for(const auto HitCharacter : HitCharacters)
	{
		FramesToCheck.Add(GetFrameToCheck(HitCharacter, HitTime));
	}

	return ShotgunConfirmHit(FramesToCheck, TraceStart, HitLocations);
}

FFramePackage ULagCompensationComponent::GetFrameToCheck(ABlasterCharacter* HitCharacter, const float HitTime) const
{
	//First check all data is valid
	// ReSharper disable once CppTooWideScope
	const bool bReturn =
		!HitCharacter ||
		!HitCharacter->GetLagCompensationComponent() ||
		!HitCharacter->GetLagCompensationComponent()->FrameHistory.GetHead() ||
		!HitCharacter->GetLagCompensationComponent()->FrameHistory.GetTail(); 

	if(bReturn)
	{
		//If any data is invalid we return no hit
		return FFramePackage();
	}

	// Frame package that we check to verify a hit
	FFramePackage FrameToCheck;
	FrameToCheck.Time = 0; //Gets rid of a Rider warning
	
	bool bShouldInterpolate = true;

	//Frame history of the hit character
	const auto& History = HitCharacter->GetLagCompensationComponent()->FrameHistory;
	const float OldestHistoryTime = History.GetTail()->GetValue().Time;
	const float NewestHistoryTime = History.GetHead()->GetValue().Time;
	if(OldestHistoryTime > HitTime)
	{
		// too far back - too much lag to do server side rewind
		return FFramePackage();
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
		// In other words, until HitTime rests between two frame packages
		if(Older->GetNextNode() == nullptr)
		{
			break;
		}

		
		Older = Older->GetNextNode();

		// If the HitTime is still greater than the older frame package being compared
		// (We're moving down the tail where time decreases)
		// Then we know that the HitTime is still further in the frame history and we set Younger = Older
		// Otherwise we know we're in between frame packages
		if(Older->GetValue().Time > HitTime)
		{
			Younger = Older;
		}
	}

	//Checking for a direct match with the older frame package, if it does we don't need to interpolate
	if(Older->GetValue().Time == HitTime)
	{
		FrameToCheck = Older->GetValue();
		bShouldInterpolate = false;
	}

	if(bShouldInterpolate)
	{
		//Interpolate between younger and older
		FrameToCheck = InterpolationBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
	}
	
	FrameToCheck.Character = HitCharacter; //Gets rid of a Rider warning
	return FrameToCheck;
}


FFramePackage ULagCompensationComponent::InterpolationBetweenFrames(const FFramePackage& OlderFrame,
                                                                    const FFramePackage& YoungerFrame, const float HitTime) const
{
	// Younger time is > Older time because time increases as the game goes on
	const float Distance = YoungerFrame.Time - OlderFrame.Time;

	//Confines the distance between the older frame and the claimed hit time between frames to a value between 0 and 1
	const float InterpolationFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.0f, 1.0f);

	FFramePackage InterpolationFramePackage;
	InterpolationFramePackage.Time = HitTime;
	InterpolationFramePackage.Character = OlderFrame.Character == nullptr ? YoungerFrame.Character : OlderFrame.Character;

	for(auto& YoungerPair : YoungerFrame.HitBoxInfo)
	{
		const FName& BoneName = YoungerPair.Key;

		//Make sure the frame hit box info matches
		if(!OlderFrame.HitBoxInfo.Contains(BoneName))
		{
			continue;
		}

		//Retrieve the capsule information for both frames
		const FCapsuleInformation& YoungerCapsule = YoungerPair.Value;
		const FCapsuleInformation& OlderCapsule = OlderFrame.HitBoxInfo[BoneName];
		
		FCapsuleInformation InterpolationCapsuleInformation;

		//Do the interpolation with a Lerp
		InterpolationCapsuleInformation.Location = FMath::Lerp(OlderCapsule.Location, YoungerCapsule.Location, InterpolationFraction);
		InterpolationCapsuleInformation.Rotation = FMath::Lerp(OlderCapsule.Rotation, YoungerCapsule.Rotation, InterpolationFraction);

		//Half height and radius don't change, so don't need to be interpolated
		InterpolationCapsuleInformation.HalfHeight = YoungerCapsule.HalfHeight;
		InterpolationCapsuleInformation.Radius = YoungerCapsule.Radius;

		InterpolationFramePackage.HitBoxInfo.Add(BoneName, InterpolationCapsuleInformation);
	}

	return InterpolationFramePackage;
}

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package,
                                                              ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	UWorld* World = GetWorld();
	if(!HitCharacter || !World)
	{
		//If this data is invalid we don't need to do anything, no hit
		return FServerSideRewindResult({false, false});
	}

	FFramePackage CurrentFrame;

	// Cache and move the hit boxes of the hit character. Caching enables us to move the hit boxes BACK to where they were
	// before we attempted to confirm the hit
	CacheHitBoxPositions(HitCharacter, CurrentFrame);
	MoveHitBoxes(HitCharacter, Package);
	
	//Mesh can get in the way of collision checks, so disable it for the moment
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	//Extend the trace end a little bit as the hit location will be on the surface of the mesh
	FHitResult ConfirmHitResult;
	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

	// Determine the head capsule component and enable it if found
	// If no head capsule found we can't check for a head shot, so move on
	if(UCapsuleComponent* HeadCapsule = HitCharacter->HitCollisionCapsules[HitCharacter->GetHeadHitBoxName()])
	{
		HeadCapsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		HeadCapsule->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
	
		World->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox);

		if(ConfirmHitResult.bBlockingHit) //Hit the head, return
		{
			ResetHitBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

			UE_LOG(LogTemp, Warning, TEXT("Confirmed headshot SSR"));
			return FServerSideRewindResult{true, true};
		}
	}

	//Past this point we didn't get a head shot, enable the remaining hit boxes
	for(const auto & CapsulePair : HitCharacter->HitCollisionCapsules)
	{
		if(CapsulePair.Value == nullptr)
		{
			continue;
		}

		CapsulePair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CapsulePair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
	}	

	//Perform the line trace now that all hit boxes are enabled
	World->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox);

	if(ConfirmHitResult.bBlockingHit) //Got a hit, not a headshot
	{
		
		ResetHitBoxes(HitCharacter, CurrentFrame);
		EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

		UE_LOG(LogTemp, Warning, TEXT("Confirmed hit SSR"));
		return FServerSideRewindResult{true, false};
	}

	//Past this point, we didn't score a hit so reset the hit boxes, character mesh collision, and return no hit
	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	return FServerSideRewindResult{false, false};
}

FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	UWorld* World = GetWorld();
	if(!HitCharacter || !World)
	{
		//If this data is invalid we don't need to do anything, no hit
		return FServerSideRewindResult({false, false});
	}

	FFramePackage CurrentFrame;

	// Cache and move the hit boxes of the hit character. Caching enables us to move the hit boxes BACK to where they were
	// before we attempted to confirm the hit
	CacheHitBoxPositions(HitCharacter, CurrentFrame);
	MoveHitBoxes(HitCharacter, Package);
	
	//Mesh can get in the way of collision checks, so disable it for the moment
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	FPredictProjectilePathParams PathParams;
	PathParams.bTraceWithChannel = true;
	PathParams.bTraceWithCollision = true;
	// PathParams.DrawDebugTime = 5.0f;
	// PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	PathParams.LaunchVelocity = InitialVelocity;
	PathParams.MaxSimTime = MaxRecordTime;
	PathParams.ProjectileRadius = 5.0f;
	PathParams.SimFrequency = 15.0f;
	PathParams.StartLocation = TraceStart;
	PathParams.TraceChannel = ECC_HitBox;
	PathParams.ActorsToIgnore.Add(GetOwner());

	FPredictProjectilePathResult PathResult;
	
	// Determine the head capsule component and enable it if found
	// If no head capsule found we can't check for a head shot, so move on
	if(UCapsuleComponent* HeadCapsule = HitCharacter->HitCollisionCapsules[HitCharacter->GetHeadHitBoxName()])
	{
		HeadCapsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		HeadCapsule->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);		

		//Perform projectile prediction on the head box
		UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);		

		if(PathResult.HitResult.bBlockingHit) //Hit the head, return
		{
			ResetHitBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

			UE_LOG(LogTemp, Warning, TEXT("Confirmed headshot projectile SSR"));
			return FServerSideRewindResult{true, true};
		}
	}

	//Past this point we didn't get a head shot, enable the remaining hit boxes
	for(const auto & CapsulePair : HitCharacter->HitCollisionCapsules)
	{
		if(CapsulePair.Value == nullptr)
		{
			continue;
		}

		CapsulePair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CapsulePair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
	}	

	//Perform the projectile prediction now that all hit boxes are enabled
	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

	if(PathResult.HitResult.bBlockingHit) //Got a hit, not a headshot
	{		
		ResetHitBoxes(HitCharacter, CurrentFrame);
		EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

		UE_LOG(LogTemp, Warning, TEXT("Confirmed hit Projectile SSR"));
		return FServerSideRewindResult{true, false};
	}

	//Past this point, we didn't score a hit so reset the hit boxes, character mesh collision, and return no hit
	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	return FServerSideRewindResult{false, false};
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmHit(const TArray<FFramePackage>& FramePackages,
                                                                            const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations)
{
	FShotgunServerSideRewindResult ShotgunResult;

	UWorld* World = GetWorld();
	if(!World)
	{
		//Don't bother if no valid world
		return ShotgunResult;
	}

	//Gets the packages with a valid character pointer
	TArray<FFramePackage> ValidFramePackages;
	for(auto& Frame : FramePackages)
	{
		if(Frame.Character)
		{
			ValidFramePackages.Add(Frame);
		}
	}

	//Cached character hit box data
	TArray<FFramePackage> CurrentFrames;

	// Iterate the valid frame packages and cache all the initial hit box positions
	for(auto& Frame : ValidFramePackages)
	{
		FFramePackage CurrentFrame;
		CacheHitBoxPositions(Frame.Character, CurrentFrame);
		MoveHitBoxes(Frame.Character, Frame);
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::NoCollision);

		CurrentFrames.Add(CurrentFrame);
	}

	//Enable the head collision box for all valid frame packages
	for(auto& Frame : ValidFramePackages)
	{		
		// Determine the head capsule component and enable it if found
		// If no head capsule found we can't check for a head shot, so move on
		if(UCapsuleComponent* HeadCapsule = Frame.Character->HitCollisionCapsules[Frame.Character->GetHeadHitBoxName()])
		{
			HeadCapsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			HeadCapsule->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
		}
	}

	// Iterate each hit location and perform the line trace to find collisions with the characters head hit boxes
	for(auto& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

		World->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox);

		//Store any hits in the headshot TMap of the shotgun result
		if(ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor()))
		{			
			if(!ShotgunResult.HeadShots.Contains(BlasterCharacter))
			{
				ShotgunResult.HeadShots.Emplace(BlasterCharacter, 1);
			}
			else
			{
				ShotgunResult.HeadShots[BlasterCharacter]++;
			}
		}
	}

	//Enable collision for all hit boxes except for the head collision box
	for(auto& Frame : ValidFramePackages)
	{
		for(const auto & CapsulePair : Frame.Character->HitCollisionCapsules)
		{
			if(CapsulePair.Value == nullptr)
			{
				continue;
			}

			CapsulePair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			CapsulePair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
		}

		if(UCapsuleComponent* HeadCapsule = Frame.Character->HitCollisionCapsules[Frame.Character->GetHeadHitBoxName()])
		{
			HeadCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	//Do the line trace again against other hit box collisions
	for(auto& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

		World->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox);
		
		if(ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor()))
		{
			if(!ShotgunResult.BodyShots.Contains(BlasterCharacter))
			{
				ShotgunResult.BodyShots.Emplace(BlasterCharacter, 1);
			}
			else
			{
				ShotgunResult.BodyShots[BlasterCharacter]++;
			}
		}
	}

	//Finally reset the hit boxes for each character, and re-enable mesh collisions
	for(auto& Frame : CurrentFrames)
	{
		ResetHitBoxes(Frame.Character, Frame);
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::QueryAndPhysics);		
	}
	
	return ShotgunResult;
}


// ReSharper disable once CppMemberFunctionMayBeStatic
void ULagCompensationComponent::CacheHitBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
	if(!HitCharacter)
	{
		return;
	}

	OutFramePackage.Character = HitCharacter;

	for(const auto & CapsulePair : HitCharacter->HitCollisionCapsules)
	{
		if(CapsulePair.Value == nullptr)
		{
			continue;
		}
		
		FCapsuleInformation CapsuleInformation;
		CapsuleInformation.Location = CapsulePair.Value->GetComponentLocation();
		CapsuleInformation.Rotation = CapsulePair.Value->GetComponentRotation();
		CapsuleInformation.Radius = CapsulePair.Value->GetUnscaledCapsuleRadius();
		CapsuleInformation.HalfHeight = CapsulePair.Value->GetUnscaledCapsuleHalfHeight();
		
		OutFramePackage.HitBoxInfo.Add(CapsulePair.Key, CapsuleInformation);
	}
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void ULagCompensationComponent::MoveHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
	if(!HitCharacter)
	{
		return;
	}
	
	for(const auto & CapsulePair : HitCharacter->HitCollisionCapsules)
	{
		if(CapsulePair.Value == nullptr || !Package.HitBoxInfo.Contains(CapsulePair.Key))
		{
			continue;
		}

		CapsulePair.Value->SetWorldLocation(Package.HitBoxInfo[CapsulePair.Key].Location);
		CapsulePair.Value->SetWorldRotation(Package.HitBoxInfo[CapsulePair.Key].Rotation);
		CapsulePair.Value->SetCapsuleRadius(Package.HitBoxInfo[CapsulePair.Key].Radius);
		CapsulePair.Value->SetCapsuleHalfHeight(Package.HitBoxInfo[CapsulePair.Key].HalfHeight);
	}
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void ULagCompensationComponent::ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
	if(!HitCharacter)
	{
		return;
	}
	
	for(const auto & CapsulePair : HitCharacter->HitCollisionCapsules)
	{
		if(CapsulePair.Value == nullptr || !Package.HitBoxInfo.Contains(CapsulePair.Key))
		{
			continue;
		}

		CapsulePair.Value->SetWorldLocation(Package.HitBoxInfo[CapsulePair.Key].Location);
		CapsulePair.Value->SetWorldRotation(Package.HitBoxInfo[CapsulePair.Key].Rotation);
		CapsulePair.Value->SetCapsuleRadius(Package.HitBoxInfo[CapsulePair.Key].Radius);
		CapsulePair.Value->SetCapsuleHalfHeight(Package.HitBoxInfo[CapsulePair.Key].HalfHeight);
		CapsulePair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}	
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void ULagCompensationComponent::EnableCharacterMeshCollision(const ABlasterCharacter* HitCharacter,
                                                             ECollisionEnabled::Type InCollisionEnabled)
{
	if(!HitCharacter || !HitCharacter->GetMesh())
	{
		return;
	}
	
	HitCharacter->GetMesh()->SetCollisionEnabled(InCollisionEnabled);
}