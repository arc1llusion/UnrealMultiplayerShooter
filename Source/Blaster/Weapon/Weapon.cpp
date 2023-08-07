// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"

#include "Casing.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/WidgetComponent.h"
#include "Animation/AnimationAsset.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/HUD/OverheadWidget.h"
#include "Blaster/HUD/PickupWidget.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Net/UnrealNetwork.h"


// Sets default values
AWeapon::AWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if(PickupWidget)
	{
		PickupWidget->SetVisibility(false);
	}

	if(HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME(AWeapon, Ammo);
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	
	if(!Owner)
	{
		BlasterOwnerCharacter = nullptr;
		BlasterOwnerController = nullptr;
	}
	else
	{
		SetHUDWeaponAmmo();
	}
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                              UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		BlasterCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex)
{
	if(ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		BlasterCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::SetHUDWeaponAmmo()
{
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if(BlasterOwnerCharacter)
	{
		BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller) : BlasterOwnerController;

		if(BlasterOwnerController)
		{
			BlasterOwnerController->SetHUDWeaponAmmo(Ammo);
		}
	}
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, AmmoCapacity);
	
	SetHUDWeaponAmmo();
}

void AWeapon::OnRep_Ammo()
{
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if(BlasterOwnerCharacter && BlasterOwnerCharacter->GetCombat() && IsFull())
	{
		BlasterOwnerCharacter->GetCombat()->JumpToShotgunEnd();
	}
	SetHUDWeaponAmmo();
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	
	switch(WeaponState)
    {
    	case EWeaponState::EWS_Equipped:
    		ShowPickupWidget(false);
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			WeaponMesh->SetSimulatePhysics(false);
			WeaponMesh->SetEnableGravity(false);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			SetReplicateMovement(false);
			if(WeaponType == EWeaponType::EWT_SubMachineGun)
			{
				WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				WeaponMesh->SetEnableGravity(true);
				WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			}
    		break;
		case EWeaponState::EWS_Dropped:
			if(HasAuthority())
			{
				AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);	
			}
			WeaponMesh->SetSimulatePhysics(true);
			WeaponMesh->SetEnableGravity(true);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			SetReplicateMovement(true);
			WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
			WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
			WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
			break;
    	default:
    		break;
    }
}

FHUDPackage AWeapon::GetHUDPackage(float CrosshairRangeFactor, FLinearColor CrosshairsColor) const
{
	return FHUDPackage
			{
				CrosshairsCenter,
				CrosshairsLeft,
				CrosshairsRight,
				CrosshairsTop,
				CrosshairsBottom,
				CrosshairRangeFactor,
				CrosshairsColor
			};
}

void AWeapon::OnRep_WeaponState()
{
	switch(WeaponState)
	{
		case EWeaponState::EWS_Equipped:
			ShowPickupWidget(false);
			WeaponMesh->SetSimulatePhysics(false);
			WeaponMesh->SetEnableGravity(false);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			SetReplicateMovement(false);
			if(WeaponType == EWeaponType::EWT_SubMachineGun)
			{
				WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				WeaponMesh->SetEnableGravity(true);
				WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			}
			break;
		case EWeaponState::EWS_Dropped:
			WeaponMesh->SetSimulatePhysics(true);
			WeaponMesh->SetEnableGravity(true);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			SetReplicateMovement(true);
			WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
			WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
			WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
			break;
		default:
			break;
	}
}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if(PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
		if(const auto Widget = Cast<UPickupWidget>(PickupWidget->GetWidget()))
		{
			Widget->SetWeaponText(WeaponType);
		}
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if(FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}

	DispenseShell();
	SpendRound();
}

void AWeapon::DispenseShell() const
{
	if(CasingClass)
	{
		if(const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName(TEXT("AmmoEject"))))
		{
			const FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);

			if(UWorld* World = GetWorld())
			{
				World->SpawnActor<ACasing>(CasingClass, SocketTransform.GetLocation(), SocketTransform.GetRotation().Rotator());
			}		
		}
	}
}

void AWeapon::Drop()
{
	SetWeaponState(EWeaponState::EWS_Dropped);

	const FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);
	BlasterOwnerCharacter = nullptr;
	BlasterOwnerController = nullptr;
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, AmmoCapacity);
	SetHUDWeaponAmmo();
}

bool AWeapon::IsEmpty() const
{
	return Ammo <= 0;
}

bool AWeapon::IsFull() const
{
	return Ammo == AmmoCapacity;
}

