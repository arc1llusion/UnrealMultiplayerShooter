// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BlasterCharacter.h"
#include "Blaster/Weapon/Weapon.h"
#include "Kismet/KismetMathLibrary.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if(!BlasterCharacter)
	{
		BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
	}

	if(!BlasterCharacter)
	{
		return;
	}

	SetAnimationVariables();
	CalculateYawOffset(DeltaSeconds);
	CalculateLean(DeltaSeconds);
	ApplyToWeaponSocket(DeltaSeconds);
}

void UBlasterAnimInstance::SetAnimationVariables()
{
	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z = 0.0f;
	Speed = Velocity.Size();

	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.0f ? true : false;
	bIsWeaponEquipped = BlasterCharacter->IsWeaponEquipped();
	EquippedWeapon = BlasterCharacter->GetEquippedWeapon();
	bIsCrouched = BlasterCharacter->bIsCrouched;
	bAiming = BlasterCharacter->IsAiming();
	TurningInPlace = BlasterCharacter->GetTurningInPlace();
	bRotateRootBone = BlasterCharacter->ShouldRotateRootBone();
	bEliminated = BlasterCharacter->IsEliminated();

	AimOffsetYaw = BlasterCharacter->GetAimOffsetYaw();
	AimOffsetPitch = BlasterCharacter->GetAimOffsetPitch();
}

void UBlasterAnimInstance::CalculateYawOffset(float DeltaSeconds)
{
	//Offset Yaw for Strafing
	const FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();
	const FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
	const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaSeconds, 6.0f);
	YawOffset = DeltaRotation.Yaw;
}

void UBlasterAnimInstance::CalculateLean(float DeltaSeconds)
{
	//Lean Calculation
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaSeconds;
	const float Interpolation = FMath::FInterpTo(Lean, Target, DeltaSeconds, 6.0f);
	Lean = FMath::Clamp(Interpolation, -180.0f, 180.0f);
}

void UBlasterAnimInstance::ApplyToWeaponSocket(float DeltaSeconds)
{
	if(bIsWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(LeftHandSocket, ERelativeTransformSpace::RTS_World);

		FVector OutPosition;
		FRotator OutRotation;
		BlasterCharacter->GetMesh()->TransformToBoneSpace(
			CharacterBoneToApplyWeapon,
			LeftHandTransform.GetLocation(),
			FRotator::ZeroRotator,
			OutPosition,
			OutRotation);

		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		if(BlasterCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			const FTransform RightHandTransform = BlasterCharacter->GetMesh()->GetSocketTransform(CharacterBoneToApplyWeapon, ERelativeTransformSpace::RTS_World);
			
			const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation
			(
				RightHandTransform.GetLocation(),
				RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget())
			);

			const FVector Start = RightHandTransform.GetLocation();
			const FVector Target = RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget());
			UE_LOG(LogTemp, Warning, TEXT("Start: %s"), *Start.ToString());
			UE_LOG(LogTemp, Warning, TEXT("Target: %s"), *Target.ToString());

			UE_LOG(LogTemp, Warning, TEXT("Look At Rotation: %s"), *LookAtRotation.ToString());

			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaSeconds, 15.0f);
		}
		
		const FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName(TEXT("MuzzleFlash"), ERelativeTransformSpace::RTS_World));
		const FVector MuzzleX{FRotationMatrix{MuzzleTipTransform.GetRotation().Rotator()}.GetUnitAxis(EAxis::X)};
		DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 1000.0f, FColor::Red);
		DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), BlasterCharacter->GetHitTarget(), FColor::Orange);
	}
}
