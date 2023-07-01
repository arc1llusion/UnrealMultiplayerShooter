// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "BlasterCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
class AWeapon;

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABlasterCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;	
	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
protected:
	virtual void BeginPlay() override;
	
	/** Input **/
	void MoveAction(const FInputActionValue &Value);

	void LookAction(const FInputActionValue &Value);
	
	void EquipAction(const FInputActionValue &Value);

	void CrouchAction(const FInputActionValue &Value);

	void AimAction(const FInputActionValue &Value);
	void StopAimAction(const FInputActionValue &Value);

	void AimOffset(float DeltaTime);

	virtual void Jump() override;

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

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	AWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* Combat;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	float AimOffsetYaw;
	float AimOffsetInterpolationYaw;
	
	float AimOffsetPitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped() const;
	AWeapon* GetEquippedWeapon() const;
	
	bool IsAiming() const;

	FORCEINLINE float GetAimOffsetYaw() const { return AimOffsetYaw; }
	FORCEINLINE float GetAimOffsetPitch() const { return AimOffsetPitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
};
