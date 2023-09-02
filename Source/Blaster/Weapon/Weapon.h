// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponTypes.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

class USoundCue;
class ABlasterPlayerController;
class ABlasterCharacter;
class ACasing;
class USphereComponent;
class USkeletalMeshComponent;
class UWidgetComponent;
class UAnimationAsset;
class UTexture2D;

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EFireType : uint8
{
	EFT_HitScan UMETA(DisplayName = "Hit Scan Weapon"),
	EFT_Projectile UMETA(DisplayName = "Projectile Weapon"),
	EFT_Shotgun  UMETA(DisplayName = "Shotgun Weapon"),

	EFT_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Owner() override;
	
	void SetHUDWeaponAmmo();

	void ShowPickupWidget(bool bShowWidget);

	virtual void Fire(const TArray<FVector_NetQuantize>& HitTargets);
	virtual void Drop();

	void AddAmmo(int32 AmmoToAdd);

	void EnableCustomDepth(bool bInEnable);	
	
	FVector TraceEndWithScatter(const FVector& HitTarget) const;
	void BurstTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& Result) const;
	FVector TraceEndWithScatter(const FVector& HitTarget, const FVector& TraceStart) const;
	FORCEINLINE bool UseScatter() const { return bUseScatter; }
	
protected:
	virtual void BeginPlay() override;

	virtual void OnWeaponStateSet();
	virtual void OnWeaponStateEquipped();
	virtual void OnWeaponStateEquippedSecondary();
	virtual void OnWeaponStateDropped();
	
	UPROPERTY(EditAnywhere)
	FName MuzzleSocketFlashName{"MuzzleFlash"};
	
	void GetSocketInformation(FTransform& OutSocketTransform, FVector& OutStart) const;	

	/*
	 * Trace End With Scatter
	 */
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bShowDebugSpheres = false;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	uint32 NumberOfBurstShots = 10;

	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	virtual void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex
	);
	
	UPROPERTY(EditAnywhere)
	float Damage = 20.0f;

	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 40.0f;

	UPROPERTY(EditAnywhere, Category = "Radial Damage Falloff")
	float InnerDamageRadius = 200.0f;

	UPROPERTY(EditAnywhere, Category = "Radial Damage Falloff")
	float OuterDamageRadius = 500.0f;

	UPROPERTY(EditAnywhere, Category = "Radial Damage Falloff")
	float MinimumDamage = 10.0f;

	UPROPERTY(Replicated, EditAnywhere)
	bool bUseServerSideRewind = false;

	UPROPERTY(EditAnywhere)
	bool bUseServerSideRewindDefault = false;

	UFUNCTION()
	void OnPingTooHigh(bool bPingTooHigh);
	
	/*
	 * Stored properties
	 */
	UPROPERTY()
	ABlasterCharacter* BlasterOwnerCharacter;
	UPROPERTY()
	ABlasterPlayerController* BlasterOwnerController;

private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USphereComponent* AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	EFireType FireType;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ACasing> CasingClass;

	void DispenseShell() const;
	
	/*
	* Textures for the weapon crosshairs
	*/

	UPROPERTY(EditAnywhere, Category="Crosshairs")
	UTexture2D* CrosshairsCenter;

	UPROPERTY(EditAnywhere, Category="Crosshairs")
	UTexture2D* CrosshairsLeft;

	UPROPERTY(EditAnywhere, Category="Crosshairs")
	UTexture2D* CrosshairsRight;

	UPROPERTY(EditAnywhere, Category="Crosshairs")
	UTexture2D* CrosshairsTop;

	UPROPERTY(EditAnywhere, Category="Crosshairs")
	UTexture2D* CrosshairsBottom;

	/*
	 * Zoomed FOV while aiming
	 */

	UPROPERTY(EditAnywhere)
	float AimScaleFactor = 1.0f;

	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30;

	UPROPERTY(EditAnywhere)
	float ZoomInterpolationSpeed = 20.0f;


	/*
	 * Automatic Fire
	 */
	UPROPERTY(EditAnywhere, Category="Combat")
    float FireDelay = 0.15f;

    UPROPERTY(EditAnywhere, Category="Combat")
    bool bAutomatic = true;

	/*
	 * Ammo
	 */
	UPROPERTY(EditAnywhere)
	int32 Ammo = 0;

	UPROPERTY(EditAnywhere)
	int32 AmmoCapacity = 0;

	void SpendRound();
	
	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);

	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 AmmoToAdd);

	/**
	 * @brief Number of unprocessed server requests for Ammo
	 * Incremented in SpendRound, Decremented in ClientUpdateAmmo
	 */
	int32 UnprocessedAmmoServerRequests = 0;
	
	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;

	/*
	 * Sounds
	 */
	UPROPERTY(EditAnywhere)
	USoundCue* EquipSound;

	UPROPERTY(EditAnywhere)
	USoundCue* ZoomInSound;

	UPROPERTY(EditAnywhere)
	USoundCue* ZoomOutSound;	

	/*
	 * Respawn when dropped
	 */

	FVector StartingLocation;
	FRotator StartingRotation;

	FTimerHandle RespawnOnDropTimer;

	void StartRespawnOnDrop();
	void RespawnWeapon();
	void ClearRespawnTimer();

	bool bIsDefaultWeapon = false;

	bool bIsBurst = false;

	void AddOrRemoveHighPingDelegate(bool bAddDelegate);

public:
	void SetWeaponState(EWeaponState State);
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE UWidgetComponent* GetPickupWidget() const { return PickupWidget; }

	FORCEINLINE USoundCue* GetZoomInSound() const { return ZoomInSound; }
	FORCEINLINE USoundCue* GetZoomOutSound() const { return ZoomOutSound; }

	FHUDPackage GetHUDPackage(float CrosshairRangeFactor, FLinearColor CrosshairsColor) const;

	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpolationSpeed() const { return ZoomInterpolationSpeed; }

	FORCEINLINE float GetFireDelay() const { return FireDelay; }
	FORCEINLINE bool IsAutomatic() const { return bAutomatic; }

	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE EFireType GetFireType() const { return FireType; }

	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetAmmoCapacity() const { return AmmoCapacity; }

	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE float GetHeadShotDamage() const { return HeadShotDamage; }

	bool IsEmpty() const;
	bool IsFull() const;

	FORCEINLINE USoundCue* GetEquippedSound() const { return EquipSound; }

	FORCEINLINE bool IsDefaultWeapon() const { return bIsDefaultWeapon; }
	FORCEINLINE void SetIsDefaultWeapon(const bool bInDefaultWeapon) { bIsDefaultWeapon = bInDefaultWeapon; }

	FORCEINLINE float GetAimScaleFactor() const { return AimScaleFactor; }

	FORCEINLINE bool IsBurst() const { return bIsBurst; }
};