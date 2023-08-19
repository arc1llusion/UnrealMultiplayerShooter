// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "Components/ActorComponent.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "CombatComponent.generated.h"

class USoundCue;
class AProjectile;
class ABlasterHUD;
class ABlasterPlayerController;
class ABlasterCharacter;
class AWeapon;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	friend class ABlasterCharacter;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void EquipWeapon(AWeapon* WeaponToEquip);
	void SwapWeapons();
	bool ShouldSwapWeapons() const;

	void Reload();
	void UpdateCarriedAmmo();

	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();

	void JumpToShotgunEnd();

	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();

	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();

	UFUNCTION(Server, Reliable)
	void ServerLaunchGrenade(const FVector_NetQuantize Target);

	FORCEINLINE int32 GetGrenades() const { return Grenades; }

	void PickupAmmo(EWeaponType WeaponType, int32 InAmmoAmount);
	
	void SetMovementSpeed(float InBaseSpeed, float InAimSpeed);

	float GetWeaponAimScaleFactor();

protected:
	virtual void BeginPlay() override;
	
	void SetAiming(bool bInAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bInAiming);

	void DropEquippedWeapon() const;

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UFUNCTION()
	void OnRep_SecondaryWeapon();
	
	void Fire();

	void FireProjectileWeapon();
	void FireHitScanWeapon();
	void FireShotgunWeapon();
	
	void LocalFire(const FVector_NetQuantize& TraceHitTarget);
	void LocalShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);

	void FireButtonPressed(bool bPressed);

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(Server, Reliable)
	void ServerShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);
	
	void TraceUnderCrosshairs(FHitResult &TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	//Happens on both server and clients
	void HandleReload();

	int32 AmountToReload();

	void ThrowGrenade();

	UPROPERTY(EditAnywhere)
	USoundCue* ThrowGrenadeSound;

	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();

	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile> GrenadeClass;

	void EquipPrimaryWeapon(AWeapon* WeaponToEquip);
	void EquipSecondaryWeapon(AWeapon* WeaponToEquip);

private:
	bool GetCrosshairWorldVector(FVector& CrosshairWorldPosition, FVector& CrosshairWorldDirection) const;
	void PerformLineTrace(FHitResult& TraceHitResult, const FVector& CrossHairWorldPosition, const FVector& CrossHairWorldDirection);
	
	void AttachActorToRightHandSocket(AActor* ActorToAttach) const;
	void AttachActorToLeftHandSocket(AActor* ActorToAttach) const;
	void AttachActorToSecondaryWeaponSocket(AActor* ActorToAttach) const;

	void UpdateAmmoValues();
	void UpdateShotgunAmmoValues();
	
	void PlayEquippedWeaponSound(const AWeapon* WeaponToEquip) const;
	void ReloadIfEmpty();	

	void ShowAttachedGrenade(bool bShowGrenade) const;

private:
	UPROPERTY()
	ABlasterCharacter* Character;

	UPROPERTY()
	ABlasterPlayerController* Controller;

	UPROPERTY()
	ABlasterHUD* HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	AWeapon* SecondaryWeapon;

	UPROPERTY(EditAnywhere, Category = "Sockets")
	FName RightHandSocketName{TEXT("RightHandSocket")};
	UPROPERTY(EditAnywhere, Category = "Sockets")
	FName LeftHandSocketName{TEXT("LeftHandSocket")};
	UPROPERTY(EditAnywhere, Category = "Sockets")
	FName PistolSocketName{TEXT("PistolSocket")};
	UPROPERTY(EditAnywhere, Category = "Sockets")
	FName SecondaryWeaponSocketName{TEXT("SecondaryWeaponSocket")};

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed{600.0f};

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed{450.0f};

	bool bFireButtonPressed;

	UPROPERTY(EditAnywhere)
	float StartTraceBuffer = 100.0f;

	/*
	 * HUD and Crosshairs
	 */
	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;
	float CrosshairOnTargetFactor;

	bool bOnTarget;
	FVector HitTarget;

	UPROPERTY(EditAnywhere, Category="Crosshairs")
	float CrosshairFallingScaleFactor = 2.25f;
	UPROPERTY(EditAnywhere, Category="Crosshairs")
	float CrosshairFallingInterpolationSpeed = 2.25f;
	UPROPERTY(EditAnywhere, Category="Crosshairs")
	float CrosshairFallingReturnInterpolationSpeed = 30.0f;
	UPROPERTY(EditAnywhere, Category="Crosshairs")
	float CrosshairAimingScaleFactor = 0.38f;
	UPROPERTY(EditAnywhere, Category="Crosshairs")
	float CrosshairAimingInterpolationSpeed = 30.0f;
	UPROPERTY(EditAnywhere, Category="Crosshairs")
	float CrosshairAimingReturnInterpolationSpeed = 30.0f;
	UPROPERTY(EditAnywhere, Category="Crosshairs")
	float CrosshairShootScaleFactor = 0.75f;
	UPROPERTY(EditAnywhere, Category="Crosshairs")
	float CrosshairShootReturnInterpolationSpeed = 40.0f;
	UPROPERTY(EditAnywhere, Category="Crosshairs")
	float CrosshairOnTargetScaleFactor = 0.10f;
	UPROPERTY(EditAnywhere, Category="Crosshairs")
	float CrosshairOnTargetInterpolationSpeed = 30.0f;
	UPROPERTY(EditAnywhere, Category="Crosshairs")
	float CrosshairOnTargetReturnInterpolationSpeed = 30.0f;
	

	FHUDPackage HUDPackage;

	/*
	 * Aiming and FOV
	 */

	//Field of view when not aiming; set to the camera's base FOV in BeginPlay
	float DefaultFOV;

	float CurrentFOV;

	UPROPERTY(EditAnywhere, Category="Combat")
	float ZoomedFOV = 30.0f;

	UPROPERTY(EditAnywhere, Category="Combat")
	float ZoomInterpolationSpeed = 20.0f;

	void InterpolateFOV(float DeltaTime);

	/*
	 * Automatic Fire
	 */
	FTimerHandle FireTimer;

	bool bCanFire = true;
	
	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire() const;

	//Carried ammo for the currently equipped weapon
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo = 0;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	TMap<EWeaponType, int32> CarriedAmmoMap;

	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 500;

	UPROPERTY(EditAnywhere)
	TMap<EWeaponType, int32> StartingAmmoMap;
	
	void InitializeCarriedAmmo();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Grenades)
	int32 Grenades = 4;

	UFUNCTION()
	void OnRep_Grenades();

	UPROPERTY(EditAnywhere)
	int32 GrenadeCapacity = 4;

	void UpdateHUDGrenades();
};