// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Weapon/Weapon.h"
#include "Engine/SkeletalMeshSocket.h" 
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
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
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FHitResult HitResult;
	TraceUnderCrossHairs(HitResult);
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
		ServerFire();
	}
}

void UCombatComponent::TraceUnderCrossHairs(FHitResult& TraceHitResult)
{
	if(!GEngine || !GEngine->GameViewport)
	{
		return;
	}
	
	FVector2d ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);

	const FVector2d CrossHairLocation(ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f);

	FVector CrossHairWorldPosition;
	FVector CrossHairWorldDirection;
	bool bScreenToWorldSuccessful = UGameplayStatics::DeprojectScreenToWorld(
										UGameplayStatics::GetPlayerController(this, 0),
										CrossHairLocation,
										CrossHairWorldPosition,
										CrossHairWorldDirection);

	if(bScreenToWorldSuccessful)
	{
		FVector Start = CrossHairWorldPosition;
		FVector End = Start + CrossHairWorldDirection * TRACE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(
					TraceHitResult,
					Start,
					End,
					ECollisionChannel::ECC_Visibility);

		if(!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
			HitTarget = End;
		}
		else
		{
			HitTarget = TraceHitResult.ImpactPoint;
			
			DrawDebugSphere(
				GetWorld(),
				TraceHitResult.ImpactPoint,
				12.0f,
				12,
				FColor::Red);
		}
	}
}

void UCombatComponent::ServerFire_Implementation()
{
	MulticastFire();
}

void UCombatComponent::MulticastFire_Implementation()
{	
	if(!EquippedWeapon || !Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("CombatComponent Servr RPC - No Character or equipped weapon"));
		return;
	}

	Character->PlayFireMontage(bAiming);
	EquippedWeapon->Fire(HitTarget);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if(!Character || !WeaponToEquip)
	{
		return;
	}
	
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName(TEXT("RightHandSocket")));
	if(HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}

	EquippedWeapon->SetOwner(Character);
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

