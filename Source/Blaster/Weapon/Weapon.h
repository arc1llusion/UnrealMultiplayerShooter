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

	virtual void Fire(const FVector& HitTarget);
	void Drop();

	void AddAmmo(int32 AmmoToAdd);

	void EnableCustomDepth(bool bInEnable);
	
protected:
	virtual void BeginPlay() override;

	virtual void OnWeaponStateSet();
	virtual void OnWeaponStateEquipped();
	virtual void OnWeaponStateEquippedSecondary();
	virtual void OnWeaponStateDropped();
	

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

private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USphereComponent* AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

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
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Ammo)
	int32 Ammo = 0;

	UFUNCTION()
	void OnRep_Ammo();

	void SpendRound();

	UPROPERTY(EditAnywhere)
	int32 AmmoCapacity = 0;

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
	 * Stored properties
	 */
	UPROPERTY()
	ABlasterCharacter* BlasterOwnerCharacter;
	UPROPERTY()
	ABlasterPlayerController* BlasterOwnerController;

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

public:
	void SetWeaponState(EWeaponState State);
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	FORCEINLINE USoundCue* GetZoomInSound() const { return ZoomInSound; }
	FORCEINLINE USoundCue* GetZoomOutSound() const { return ZoomOutSound; }

	FHUDPackage GetHUDPackage(float CrosshairRangeFactor, FLinearColor CrosshairsColor) const;

	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpolationSpeed() const { return ZoomInterpolationSpeed; }

	FORCEINLINE float GetFireDelay() const { return FireDelay; }
	FORCEINLINE bool IsAutomatic() const { return bAutomatic; }

	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }

	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetAmmoCapacity() const { return AmmoCapacity; }

	bool IsEmpty() const;
	bool IsFull() const;

	FORCEINLINE USoundCue* GetEquippedSound() const { return EquipSound; }

	FORCEINLINE bool IsDefaultWeapon() const { return bIsDefaultWeapon; }
	FORCEINLINE void SetIsDefaultWeapon(const bool bInDefaultWeapon) { bIsDefaultWeapon = bInDefaultWeapon; }
};