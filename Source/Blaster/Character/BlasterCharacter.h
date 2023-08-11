// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "Blaster/Interfaces//InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "BlasterCharacter.generated.h"

class UBuffComponent;
class USphereComponent;
class UOverheadWidget;
class ABlasterPlayerState;
class USoundCue;
class ABlasterPlayerController;
class UInputMappingContext;
class UInputAction;
class UAnimMontage;
class USpringArmComponent;
class UCameraComponent;
class UWidgetComponent;
class UCombatComponent;
class AWeapon;

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;

	void PlayFireMontage(bool bAiming);
	
	void PlayReloadMontage();
	void JumpToReloadMontageSection(const FName& SectionName) const;
	
	void PlayEliminationMontage();

	void PlayThrowGrenadeMontage();

	virtual void OnRep_ReplicatedMovement() override;

	//Only called on the server
	void Eliminate();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastEliminate();

	virtual void Destroyed() override;
protected:

	virtual void BeginPlay() override;

	/** Input **/
	void MoveAction(const FInputActionValue &Value);

	void LookAction(const FInputActionValue &Value);
	
	void EquipAction(const FInputActionValue &Value);

	void CrouchAction(const FInputActionValue &Value);

	void AimAction(const FInputActionValue &Value);
	void StopAimAction(const FInputActionValue &Value);

	void FireAction(const FInputActionValue &Value);
	void StopFireAction(const FInputActionValue &Value);

	void ChangeCharacterAction(const FInputActionValue &Value);
	
	void ReloadAction(const FInputActionValue &Value);

	void ThrowGrenadeAction(const FInputActionValue &Value);

	void AimOffset(float DeltaTime);
	
	void SimProxiesTurn();

	virtual void Jump() override;

	void PlayHitReactMontage();

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser);

	UFUNCTION()
	void KillZOverlap(AActor* OverlappedActor, AActor* OtherActor);

	//Poll for any relevant classes and initialize our HUD
	void PollInit();
	void RotateInPlace(float DeltaTime);

	/** Input Assets **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext *DefaultMappingContext;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction *MoveActionAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction *LookActionAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction *JumpActionAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction *EquipActionAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction *CrouchInputAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction *AimInputAsset;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    UInputAction *FireInputAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction *ChangeCharacterAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction *ReloadInputAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction *ThrowGrenadeInputAsset;

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, Category = "Overhead Collision")
	USphereComponent* OverheadCollisionSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	AWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCombatComponent* Combat;

	UPROPERTY(VisibleAnywhere)
	UBuffComponent* Buff;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	float AimOffsetYaw;
	float AimOffsetInterpolationYaw;
	
	float AimOffsetPitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* FireWeaponMontage;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* HitReactMontage;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* EliminationMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* ThrowGrenadeMontage;

	void HideCharacterIfCameraClose();

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.0f;

	bool bRotateRootBone;
	
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;

	float TimeSinceLastMovementReplication = 0.0f;

	/*
	 * Player Health
	 */

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(ReplicatedUsing= OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.0f;

	UFUNCTION()
	void OnRep_Health();	
	void UpdateHUDHealth();

	UPROPERTY()
	ABlasterPlayerController* BlasterPlayerController;

	bool bEliminated = false;

	FTimerHandle EliminateHandle;
	
	UPROPERTY(EditDefaultsOnly)
	float EliminateDelay = 3.0f;

	UFUNCTION()
	void EliminateTimerFinished();

	/*
	 * Dissolve Effect
	 */

	UPROPERTY(VisibleAnywhere, Category = "Elimination")
	UTimelineComponent* DissolveTimeline;	
	FOnTimelineFloat DissolveTrack;
	
	UPROPERTY(EditAnywhere, Category = "Elimination")
	UCurveFloat* DissolveCurve;

	// Dynamic Instance we can change at run time
	UPROPERTY(VisibleAnywhere, Category = "Elimination")
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	//Material instance set on the blueprint, used with the dynamic material instance
	UPROPERTY(EditAnywhere, Category = "Elimination")
	UMaterialInstance* DissolveMaterialInstance;

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();

	/*
	 * Elimination Bot
	 */

	UPROPERTY(EditAnywhere, Category = "Elimination")
	UParticleSystem* EliminationBotEffect;

	UPROPERTY(VisibleAnywhere, Category = "Elimination")
	UParticleSystemComponent* EliminationBotComponent;

	UPROPERTY(EditAnywhere, Category = "Elimination")
	USoundCue* EliminationBotSound;

	/*
	 * Grenade
	 */

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;

	UPROPERTY()
	ABlasterPlayerState* BlasterPlayerState;
	
public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped() const;
	AWeapon* GetEquippedWeapon() const;
	
	bool IsAiming() const;

	FORCEINLINE float GetAimOffsetYaw() const { return AimOffsetYaw; }
	FORCEINLINE float GetAimOffsetPitch() const { return AimOffsetPitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsEliminated() const { return bEliminated; }
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }

	FVector GetHitTarget() const;

	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	
	ECombatState GetCombatState() const;
	
	UFUNCTION()
	void OnOverheadOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
								const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverheadOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void SetupOverheadOverlapEvents();
	
	void SetDisplayOverheadWidget(bool bInDisplay) const;
	
	/*
	 * Player Controller Functions
	 */
	bool GetDisableGameplayFromController();
	
private:
	void CalculateAimOffsetYaw(float DeltaTime);
	void CalculateAimOffsetPitch();
	float CalculateSpeed() const;

	void StopCharacterMovement();
	void DisableCollision();

	FName DissolveParameterName{TEXT("Dissolve")};
	FName GlowParameterName{TEXT("Glow")};
	FName GrenadeSocketName{TEXT("GrenadeSocket")};

	void SpawnEliminationBot();
	
	void HideAimingScope();

	UFUNCTION()
	void OnReloadMontageEnd(UAnimMontage* AnimMontage, bool bInterrupted) const;

	UFUNCTION()
	void OnThrowGrenadeMontageEnd(UAnimMontage* AnimMontage, bool bInterrupted) const;
};
