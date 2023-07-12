// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/Weapon/Weapon.h"
#include "Engine/SkeletalMeshSocket.h" 
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "Net/UnrealNetwork.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if(Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		if(Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if(Character && Character->IsLocallyControlled() && EquippedWeapon)
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);

		HitTarget = HitResult.ImpactPoint;

		SetHUDCrosshairs(DeltaTime);
		InterpolateFOV(DeltaTime);
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if(!Character || !Character->Controller)
	{
		return;
	}

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

	if(Controller)
	{
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;

		if(HUD)
		{
			const float MaxMovementSpeed = Character->GetCharacterMovement()->IsCrouching() ?
				                               Character->GetCharacterMovement()->MaxWalkSpeedCrouched :
				                               Character->GetCharacterMovement()->GetMaxSpeed();

			const FVector2D MaxSpeedRange{0.0f, MaxMovementSpeed};
			const FVector2D VelocityMultiplierRange{0.0f, 1.0f};
			const FVector Velocity{Character->GetVelocity().X, Character->GetVelocity().Y, 0.0f};
			
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped
			(
				MaxSpeedRange,
				VelocityMultiplierRange,
				Velocity.Size()
			);

			if(Character->GetCharacterMovement()->IsFalling())
			{
				CrossHairInAirFactor = FMath::FInterpTo(CrossHairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				CrossHairInAirFactor = FMath::FInterpTo(CrossHairInAirFactor, 0.0f, DeltaTime, 30.0f);
			}

			if(bAiming)
			{
				CrossHairAimFactor = FMath::FInterpTo(CrossHairAimFactor, 0.58f, DeltaTime, 30.0f);
			}
			else
			{
				CrossHairAimFactor = FMath::FInterpTo(CrossHairAimFactor, 0.0f, DeltaTime, 30.0f);
			}

			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.0f, DeltaTime, 40.0f);
			
			if(EquippedWeapon)
			{
				const float TotalCrosshairFactor = 0.5f +
													CrosshairVelocityFactor +
													CrossHairInAirFactor -
													CrossHairAimFactor +
													CrosshairShootingFactor;

				HUDPackage = EquippedWeapon->GetHUDPackage(TotalCrosshairFactor, HUDPackage.CrosshairsColor);
				HUD->SetHUDPackage(HUDPackage);
			}
			else
			{
				HUDPackage = FHUDPackage::NullPackage;
				HUD->SetHUDPackage(HUDPackage);
			}
		}
	}
}

void UCombatComponent::SetAiming(bool bInAiming)
{
	bAiming = bInAiming;
	ServerSetAiming(bInAiming);

	if(Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bInAiming)
{
	bAiming = bInAiming;
	if(Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if(EquippedWeapon && Character)
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	if(bFireButtonPressed)
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		ServerFire(HitResult.ImpactPoint);

		if(EquippedWeapon)
		{
			CrosshairShootingFactor = 0.75f;
		}
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	if(!GEngine || !GEngine->GameViewport)
	{
		return;
	}

	FVector CrossHairWorldPosition;
	FVector CrossHairWorldDirection;
	if(GetCrosshairWorldVector(CrossHairWorldPosition, CrossHairWorldDirection))
	{
		PerformLineTrace(TraceHitResult, CrossHairWorldPosition, CrossHairWorldDirection);
	}
}

bool UCombatComponent::GetCrosshairWorldVector(FVector& CrosshairWorldPosition, FVector& CrosshairWorldDirection) const
{
	FVector2d ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);

	const FVector2d CrossHairLocation(ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f);

	return UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrossHairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection);
}

void UCombatComponent::PerformLineTrace(FHitResult& TraceHitResult, const FVector& CrossHairWorldPosition, const FVector& CrossHairWorldDirection)
{
	const FVector Start = CrossHairWorldPosition;
	const FVector End = Start + CrossHairWorldDirection * TRACE_LENGTH;

	GetWorld()->LineTraceSingleByChannel(
		TraceHitResult,
		Start,
		End,
		ECollisionChannel::ECC_Visibility);

	if(!TraceHitResult.bBlockingHit)
	{
		TraceHitResult.ImpactPoint = End;
	}

	if(TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
	{
		HUDPackage.CrosshairsColor = FLinearColor::Red;
	}
	else
	{
		HUDPackage.CrosshairsColor = FLinearColor::White;
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{	
	if(!EquippedWeapon || !Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("CombatComponent Servr RPC - No Character or equipped weapon"));
		return;
	}

	Character->PlayFireMontage(bAiming);
	EquippedWeapon->Fire(TraceHitTarget);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if(!Character || !WeaponToEquip)
	{
		return;
	}
	
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	AttachWeaponToHandSocket();

	EquippedWeapon->SetOwner(Character);
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::AttachWeaponToHandSocket() const
{
	if(const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(RightHandSocketName))
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}
}

void UCombatComponent::InterpolateFOV(float DeltaTime)
{
	if(!EquippedWeapon)
	{
		return;
	}

	if(bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpolationSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpolationSpeed);
	}

	if(Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

