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
#include "Blaster/Weapon/Projectile.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, bIsAiming);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, Grenades);
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

	UpdateHUDGrenades();
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

		if(EquippedWeapon)
		{
			CrosshairShootingFactor = CrosshairShootScaleFactor;

			switch(EquippedWeapon->GetFireType())
			{
				case EFireType::EFT_Projectile:
					FireProjectileWeapon();
					break;
				case EFireType::EFT_HitScan:
					FireHitScanWeapon();
					break;
				case EFireType::EFT_Shotgun:
					FireShotgunWeapon();
					break;
				case EFireType::EFT_MAX:
					break;
			}
		}
	
		StartFireTimer();
	}
}

void UCombatComponent::FireProjectileWeapon()
{
	if(Character && EquippedWeapon)
	{
		HitTarget = EquippedWeapon->UseScatter() ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		if(!Character->HasAuthority())
		{
			LocalFire(HitTarget);
		}
		ServerFire(HitTarget);
	}
}

void UCombatComponent::FireHitScanWeapon()
{
	if(EquippedWeapon)
	{
		HitTarget = EquippedWeapon->UseScatter() ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		if(!Character->HasAuthority())
		{
			LocalFire(HitTarget);
		}
		ServerFire(HitTarget);
	}
}

void UCombatComponent::FireShotgunWeapon()
{
	if(EquippedWeapon)
	{
		TArray<FVector_NetQuantize> HitTargets;
		EquippedWeapon->BurstTraceEndWithScatter(HitTarget, HitTargets);

		if(!Character->HasAuthority())
		{
			LocalShotgunFire(HitTargets);
		}
		ServerShotgunFire(HitTargets);
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
	if(Character && Character->IsLocallyControlled() && !Character->HasAuthority())
	{
		return;
	}
	
	LocalFire(TraceHitTarget);
}

void UCombatComponent::ServerShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	MulticastShotgunFire(TraceHitTargets);
}

void UCombatComponent::MulticastShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	if(Character && Character->IsLocallyControlled() && !Character->HasAuthority())
	{
		return;
	}
	
	LocalShotgunFire(TraceHitTargets);
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget)
{
	if(!EquippedWeapon || !Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("CombatComponent Local Fire - No Character or equipped weapon"));
		return;
	}
	
	if(Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bIsAiming);

		auto Hits = TArray<FVector_NetQuantize>();
		Hits.Add(TraceHitTarget);
		EquippedWeapon->Fire(Hits);
	}
}

void UCombatComponent::LocalShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets)
{	
	if(!EquippedWeapon || !Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("CombatComponent Local Shotgun Fire - No Character or equipped weapon"));
		return;
	}

	if(CombatState == ECombatState::ECS_Reloading || CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bIsAiming);
		EquippedWeapon->Fire(TraceHitTargets);		
		CombatState = ECombatState::ECS_Unoccupied;
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if(!Character || !WeaponToEquip || CombatState != ECombatState::ECS_Unoccupied)
	{
		return;
	}

	if(EquippedWeapon && !SecondaryWeapon)
	{
		EquipSecondaryWeapon(WeaponToEquip);
	}
	else
	{
		EquipPrimaryWeapon(WeaponToEquip);
	}	
	
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::SwapWeapons()
{
	if(CombatState != ECombatState::ECS_Unoccupied)
	{
		return;
	}
	
	AWeapon* TempWeapon = EquippedWeapon;
	EquippedWeapon = SecondaryWeapon;
	SecondaryWeapon = TempWeapon;

	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHandSocket(EquippedWeapon);
	EquippedWeapon->SetHUDWeaponAmmo();
	UpdateCarriedAmmo();
	PlayEquippedWeaponSound(EquippedWeapon);
	
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToSecondaryWeaponSocket(SecondaryWeapon);
	
}

bool UCombatComponent::ShouldSwapWeapons() const
{
	return EquippedWeapon && SecondaryWeapon;
}

void UCombatComponent::EquipPrimaryWeapon(AWeapon* WeaponToEquip)
{
	if(!WeaponToEquip)
	{
		return;
	}
	
	DropEquippedWeapon();
	
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	AttachActorToRightHandSocket(EquippedWeapon);

	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetHUDWeaponAmmo();
	UpdateCarriedAmmo();
	
	PlayEquippedWeaponSound(EquippedWeapon);
	ReloadIfEmpty();
}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* WeaponToEquip)
{
	if(!WeaponToEquip)
	{
		return;
	}
	
	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	
	AttachActorToSecondaryWeaponSocket(SecondaryWeapon);

	SecondaryWeapon->SetOwner(Character);

	PlayEquippedWeaponSound(SecondaryWeapon);
}

void UCombatComponent::DropEquippedWeapon() const
{
	if(EquippedWeapon)
	{
		EquippedWeapon->Drop();
	}
}

void UCombatComponent::PlayEquippedWeaponSound(const AWeapon* WeaponToEquip) const
{
	if(!WeaponToEquip || !WeaponToEquip->GetEquippedSound() || !Character)
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(
			this,
			WeaponToEquip->GetEquippedSound(),
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
	if(EquippedWeapon && EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

void UCombatComponent::UpdateCarriedAmmo()
{
	if(!Character || !EquippedWeapon)
	{
		return;
	}

	if(CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
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

	UpdateCarriedAmmo();
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

	UpdateCarriedAmmo();
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

void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 InAmmoAmount)
{
	if(CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + InAmmoAmount, 0, MaxCarriedAmmo);
		UpdateCarriedAmmo();
	}
	else
	{
		CarriedAmmoMap.Emplace(WeaponType, InAmmoAmount);
	}	

	if(EquippedWeapon && EquippedWeapon->IsEmpty() && EquippedWeapon->GetWeaponType() == WeaponType)
	{
		Reload();
	}
}

void UCombatComponent::SetMovementSpeed(float InBaseSpeed, float InAimSpeed)
{
	BaseWalkSpeed = InBaseSpeed;
	AimWalkSpeed = InAimSpeed;
}

float UCombatComponent::GetWeaponAimScaleFactor()
{
	if(EquippedWeapon)
	{
		return EquippedWeapon->GetAimScaleFactor();
	}

	return 1.0f;
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
				AttachActorToLeftHandSocket(EquippedWeapon);
				ShowAttachedGrenade(true);
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

void UCombatComponent::ThrowGrenadeFinished()
{
	CombatState = ECombatState::ECS_Unoccupied;
	AttachActorToRightHandSocket(EquippedWeapon);
}

void UCombatComponent::LaunchGrenade()
{
	ShowAttachedGrenade(false);
	if(Character && Character->IsLocallyControlled())
	{
		if(ThrowGrenadeSound)
		{
			UGameplayStatics::PlaySound2D(this, ThrowGrenadeSound);
		}
		
		ServerLaunchGrenade(HitTarget);
	}
}

void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize Target)
{
	if(GrenadeClass && Character && Character->GetAttachedGrenade())
	{
		const FVector StartingLocation = Character->GetAttachedGrenade()->GetComponentLocation();
		const FVector ToTarget = Target - StartingLocation;

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = Character;
		SpawnParameters.Instigator = Character;

		if(UWorld* World = GetWorld())
		{
			World->SpawnActor<AProjectile>(GrenadeClass, StartingLocation, ToTarget.Rotation(), SpawnParameters);	
		}		
	}
}

void UCombatComponent::ThrowGrenade()
{
	if(Grenades == 0 || CombatState != ECombatState::ECS_Unoccupied || !EquippedWeapon)
	{
		return;
	}
	
	CombatState = ECombatState::ECS_ThrowingGrenade;

	if(Character)
	{
		Character->PlayThrowGrenadeMontage();
		AttachActorToLeftHandSocket(EquippedWeapon);
		ShowAttachedGrenade(true);
	}

	if(Character && !Character->HasAuthority())
	{
		ServerThrowGrenade();	
	}
	if(Character && Character->HasAuthority())
	{
		Grenades = FMath::Clamp(Grenades - 1, 0, GrenadeCapacity);
		UpdateHUDGrenades();
	}
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	if(Grenades == 0)
	{
		return;
	}
	
	CombatState = ECombatState::ECS_ThrowingGrenade;
	if(Character)
	{
		Character->PlayThrowGrenadeMontage();
		AttachActorToLeftHandSocket(EquippedWeapon);
		ShowAttachedGrenade(true);
	}

	Grenades = FMath::Clamp(Grenades - 1, 0, GrenadeCapacity);
	UpdateHUDGrenades();
}

void UCombatComponent::ShowAttachedGrenade(bool bShowGrenade) const
{
	if(!Character || !Character->GetAttachedGrenade())
	{
		return;
	}
	
	Character->GetAttachedGrenade()->SetVisibility(bShowGrenade);
}

void UCombatComponent::OnRep_Grenades()
{
	UpdateHUDGrenades();
}

void UCombatComponent::UpdateHUDGrenades()
{
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if(Controller)
	{
		Controller->SetHUDGrenades(Grenades);
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if(EquippedWeapon && Character)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHandSocket(EquippedWeapon);
		
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;

		PlayEquippedWeaponSound(EquippedWeapon);

		Character->UpdateHUDAmmo();

		EquippedWeapon->EnableCustomDepth(false);
		EquippedWeapon->SetHUDWeaponAmmo();
	}
}

void UCombatComponent::OnRep_SecondaryWeapon()
{
	if(SecondaryWeapon && Character)
	{
		SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
		AttachActorToSecondaryWeaponSocket(SecondaryWeapon);		
		
		PlayEquippedWeaponSound(SecondaryWeapon);
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
	
	bIsAiming = bInAiming;
	ServerSetAiming(bInAiming);

	Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDSniperScope(bIsAiming);
		}
	}

	if(Character->IsLocallyControlled())
	{
		bIsLocalAiming = bIsAiming;
	}

	if(Character->IsLocallyControlled())
	{		
		if (bIsAiming)
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

void UCombatComponent::OnRep_Aiming()
{
	if(Character && Character->IsLocallyControlled())
	{
		bIsAiming = bIsLocalAiming;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bInAiming)
{
	bIsAiming = bInAiming;
	if(Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
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

			if(bIsAiming)
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

void UCombatComponent::AttachActorToRightHandSocket(AActor* ActorToAttach) const
{
	if(!Character || !ActorToAttach || !Character->GetMesh())
	{
		return;
	}
	
	if(const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(RightHandSocketName))
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::AttachActorToLeftHandSocket(AActor* ActorToAttach) const
{
	if(!Character || !ActorToAttach || !Character->GetMesh() || !EquippedWeapon)
	{
		return;
	}

	const bool bUsePistolSocket = EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol ||
								  EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SubMachineGun;
	
	if(const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(bUsePistolSocket ? PistolSocketName : LeftHandSocketName))
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::AttachActorToSecondaryWeaponSocket(AActor* ActorToAttach) const
{
	if(!Character || !ActorToAttach || !Character->GetMesh())
	{
		return;
	}
	
	if(const USkeletalMeshSocket* SecondaryWeaponSocket = Character->GetMesh()->GetSocketByName(SecondaryWeaponSocketName))
	{
		SecondaryWeaponSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::InterpolateFOV(float DeltaTime)
{
	if(!EquippedWeapon)
	{
		return;
	}

	if(bIsAiming)
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
