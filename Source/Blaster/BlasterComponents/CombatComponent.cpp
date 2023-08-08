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
#include "Sound/SoundCue.h"
#include "Blaster/Character/BlasterAnimInstance.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
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

		if(Character->HasAuthority())
		{
			InitializeCarriedAmmo();
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

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	if(bFireButtonPressed && EquippedWeapon)
	{
		Fire();
	}
}

void UCombatComponent::Fire()
{
	if(CanFire())
	{
		bCanFire = false;
		ServerFire(HitTarget);

		if(EquippedWeapon)
		{
			CrosshairShootingFactor = CrosshairShootScaleFactor;
		}
	
		StartFireTimer();
	}
}

void UCombatComponent::StartFireTimer()
{
	if(!EquippedWeapon || !Character)
	{
		return;
	}

	FTimerManager& TimerManager = Character->GetWorldTimerManager();
	TimerManager.SetTimer(FireTimer, this, &UCombatComponent::FireTimerFinished, EquippedWeapon->GetFireDelay());
}

void UCombatComponent::FireTimerFinished()
{
	if(!EquippedWeapon)
	{
		return;
	}
	
	bCanFire = true;
	if(bFireButtonPressed && EquippedWeapon->IsAutomatic())
	{
		Fire();
	}

	ReloadIfEmpty();
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

	if(Character && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
		CombatState = ECombatState::ECS_Unoccupied;
		return;
	}
	
	if(CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if(!Character || !WeaponToEquip || CombatState != ECombatState::ECS_Unoccupied)
	{
		return;
	}

	if(EquippedWeapon)
	{
		EquippedWeapon->Drop();
	}
	
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	AttachWeaponToHandSocket();

	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetHUDWeaponAmmo();

	if(CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	SetHUDCarriedAmmo();
	PlayEquippedWeaponSound();
	ReloadIfEmpty();
	
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::PlayEquippedWeaponSound() const
{
	if(!EquippedWeapon || !EquippedWeapon->GetEquippedSound() || !Character)
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(
			this,
			EquippedWeapon->GetEquippedSound(),
			Character->GetActorLocation());
}

void UCombatComponent::Reload()
{
	if(CarriedAmmo > 0 && CombatState == ECombatState::ECS_Unoccupied && EquippedWeapon && !EquippedWeapon->IsFull())
	{
		ServerReload();	
	}
}

void UCombatComponent::ReloadIfEmpty()
{
	if(EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

void UCombatComponent::SetHUDCarriedAmmo()
{
	if(!Character)
	{
		return;
	}
	
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if(Controller)
	{		
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if(!Character || !EquippedWeapon)
	{
		return;
	}

	CombatState = ECombatState::ECS_Reloading;
	HandleReload();
}

void UCombatComponent::FinishReloading()
{
	if(!Character)
	{
		return;
	}

	if(Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}

	if(bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::ShotgunShellReload()
{
	if(Character && Character->HasAuthority())
	{
		UpdateShotgunAmmoValues();	
	}
}


void UCombatComponent::UpdateAmmoValues()
{
	if(!EquippedWeapon)
	{
		return;
	}

	const int32 ReloadAmount = AmountToReload();
	if(CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	SetHUDCarriedAmmo();
	EquippedWeapon->AddAmmo(ReloadAmount);
}

void UCombatComponent::UpdateShotgunAmmoValues()
{
	if(!EquippedWeapon || !Character)
	{
		return;
	}
	
	if(CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	SetHUDCarriedAmmo();
	EquippedWeapon->AddAmmo(1);
	bCanFire = true;

	if(EquippedWeapon->IsFull() || CarriedAmmo == 0)
	{
		//Jump to ShotgunEnd section
		Character->JumpToReloadMontageSection(FName(TEXT("ShotgunEnd")));
	}
}

void UCombatComponent::JumpToShotgunEnd()
{
	if(!Character)
	{
		return;
	}
	
	Character->JumpToReloadMontageSection(FName(TEXT("ShotgunEnd")));
}

void UCombatComponent::ThrowGrenadeFinished()
{
	CombatState = ECombatState::ECS_Unoccupied;
}

void UCombatComponent::OnRep_CombatState()
{
	switch(CombatState)
	{
		case ECombatState::ECS_Unoccupied:
			if(bFireButtonPressed)
			{
				Fire();
			}
			break;
			
		case ECombatState::ECS_Reloading:
			HandleReload();
			break;

		case ECombatState::ECS_ThrowingGrenade:
			if(Character && !Character->IsLocallyControlled())
			{
				Character->PlayThrowGrenadeMontage();
			}
			break;

		default:
			break;
	}
}

void UCombatComponent::HandleReload()
{
	Character->PlayReloadMontage();
}

int32 UCombatComponent::AmountToReload()
{
	if(!EquippedWeapon)
	{
		return 0;
	}

	if(!CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		return 0;
	}	

	const int32 RoomInWeapon = EquippedWeapon->GetAmmoCapacity() - EquippedWeapon->GetAmmo();
	const int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	const int32 Least = FMath::Min(RoomInWeapon, AmountCarried);

	return FMath::Clamp(RoomInWeapon, 0, Least);
}

void UCombatComponent::ThrowGrenade()
{
	if(CombatState != ECombatState::ECS_Unoccupied)
	{
		return;
	}
	
	CombatState = ECombatState::ECS_ThrowingGrenade;

	if(Character)
	{
		Character->PlayThrowGrenadeMontage();
	}

	if(Character && !Character->HasAuthority())
	{
		ServerThrowGrenade();	
	}	
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	CombatState = ECombatState::ECS_ThrowingGrenade;
	if(Character)
	{
		Character->PlayThrowGrenadeMontage();
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if(EquippedWeapon && Character)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachWeaponToHandSocket();
		
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;

		PlayEquippedWeaponSound();
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
	FVector Start = CrossHairWorldPosition;

	if(Character)
	{
		const float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
		Start += CrossHairWorldDirection * (DistanceToCharacter + StartTraceBuffer);
	}
	
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
		bOnTarget = true;
		HUDPackage.CrosshairsColor = FLinearColor::Red;
	}
	else
	{
		bOnTarget = false;
		HUDPackage.CrosshairsColor = FLinearColor::White;
	}
}

void UCombatComponent::SetAiming(bool bInAiming)
{
	if(!Character || !EquippedWeapon)
	{
		return;
	}
	
	bAiming = bInAiming;
	ServerSetAiming(bInAiming);
	
	Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDSniperScope(bAiming);
		}
	}

	if(Character->IsLocallyControlled())
	{		
		if (bAiming)
		{
			if(EquippedWeapon->GetZoomInSound())
			{
				UGameplayStatics::PlaySound2D(this, EquippedWeapon->GetZoomInSound());
			}
		}
		else
		{
			if(EquippedWeapon->GetZoomOutSound())
			{
				UGameplayStatics::PlaySound2D(this, EquippedWeapon->GetZoomOutSound());
			}
		}
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
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, CrosshairFallingScaleFactor, DeltaTime, CrosshairFallingScaleFactor);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.0f, DeltaTime, CrosshairFallingReturnInterpolationSpeed);
			}

			if(bAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, CrosshairAimingScaleFactor, DeltaTime, CrosshairAimingInterpolationSpeed);
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.0f, DeltaTime, CrosshairAimingReturnInterpolationSpeed);
			}

			if(bOnTarget)
			{
				CrosshairOnTargetFactor = FMath::FInterpTo(CrosshairOnTargetFactor, CrosshairOnTargetScaleFactor, DeltaTime, CrosshairOnTargetInterpolationSpeed);
			}
			else
			{
				CrosshairOnTargetFactor = FMath::FInterpTo(CrosshairOnTargetFactor, 0.0f, DeltaTime, CrosshairOnTargetReturnInterpolationSpeed);
			}

			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.0f, DeltaTime, CrosshairShootReturnInterpolationSpeed);
			
			if(EquippedWeapon)
			{
				const float TotalCrosshairFactor = 0.5f +
													CrosshairVelocityFactor +
													CrosshairInAirFactor -
													CrosshairAimFactor +
													CrosshairShootingFactor -
													CrosshairOnTargetFactor;

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

bool UCombatComponent::CanFire() const
{
	return EquippedWeapon &&
		   !EquippedWeapon->IsEmpty() &&
		   (
		   	 (bCanFire && CombatState == ECombatState::ECS_Unoccupied) ||
		   	 (bCanFire && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
		   );
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if(Controller)
	{		
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	bool bJumpToShotgunEnd = CombatState == ECombatState::ECS_Reloading &&
											EquippedWeapon &&
											EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun &&
											CarriedAmmo == 0;

	if(bJumpToShotgunEnd)
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	for(auto& MapElement : StartingAmmoMap)
	{
		if(CarriedAmmoMap.Contains(MapElement.Key))
		{
			CarriedAmmoMap[MapElement.Key] = MapElement.Value;
		}
		else
		{
			CarriedAmmoMap.Emplace(MapElement.Key, MapElement.Value);
		}
	}
}
