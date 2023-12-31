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
#include "Blaster/HUD/PickupWidget.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Kismet/KismetMathLibrary.h"
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

	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);

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
	
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);

	StartingLocation = GetActorLocation();
	StartingRotation = GetActorRotation();

	bUseServerSideRewindDefault = bUseServerSideRewind;
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME_CONDITION(AWeapon, bUseServerSideRewind, COND_OwnerOnly);
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
		BlasterOwnerCharacter = !BlasterOwnerCharacter ? Cast<ABlasterCharacter>(Owner) : BlasterOwnerCharacter;

		if(BlasterOwnerCharacter && BlasterOwnerCharacter->GetEquippedWeapon() && BlasterOwnerCharacter->GetEquippedWeapon() == this)
		{
			SetHUDWeaponAmmo();
		}
	}
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                              UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		if(WeaponType == EWeaponType::EWT_Flag && BlasterCharacter->GetTeam() == Team)
		{
			return;
		}

		if(BlasterCharacter->IsHoldingTheFlag())
		{
			return;
		}
		
		BlasterCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex)
{
	if(ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		if(WeaponType == EWeaponType::EWT_Flag && BlasterCharacter->GetTeam() != Team)
		{
			return;
		}

		if(BlasterCharacter->IsHoldingTheFlag())
		{
			return;
		}
		
		BlasterCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::OnPingTooHigh(bool bPingTooHigh)
{
	if(bPingTooHigh)
	{
		//UE_LOG(LogTemp, Warning, TEXT("%s: Ping too high, set use server side rewind to false"), *GetActorNameOrLabel());
		bUseServerSideRewind = false;
	}
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("%s: Ping okay, set use server side rewind to: %d"), *GetActorNameOrLabel(), bUseServerSideRewindDefault);
		bUseServerSideRewind = bUseServerSideRewindDefault;
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

	if(HasAuthority())
	{
		ClientUpdateAmmo(Ammo);
	}
	else
	{
		++UnprocessedAmmoServerRequests;
	}
}

void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
	if(HasAuthority())
	{
		return;
	}
	
	Ammo = ServerAmmo;
	--UnprocessedAmmoServerRequests;

	Ammo -= UnprocessedAmmoServerRequests;
	SetHUDWeaponAmmo();
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, AmmoCapacity);
	SetHUDWeaponAmmo();

	ClientAddAmmo(AmmoToAdd);
}

void AWeapon::ClientAddAmmo_Implementation(int32 AmmoToAdd)
{
	if(HasAuthority())
	{
		return;
	}
	
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, AmmoCapacity);

	BlasterOwnerCharacter = !BlasterOwnerCharacter ? Cast<ABlasterCharacter>(Owner) : BlasterOwnerCharacter;
	if(BlasterOwnerCharacter && BlasterOwnerCharacter->GetCombat() && IsFull())
	{
		BlasterOwnerCharacter->GetCombat()->JumpToShotgunEnd();
	}
	
	SetHUDWeaponAmmo();
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;	
	OnWeaponStateSet();
}

void AWeapon::OnRep_WeaponState()
{
	OnWeaponStateSet();
}

void AWeapon::OnWeaponStateSet()
{
	switch(WeaponState)
	{
		case EWeaponState::EWS_Equipped:
			OnWeaponStateEquipped();
			break;
		case EWeaponState::EWS_EquippedSecondary:
			OnWeaponStateEquippedSecondary();
			break;
		case EWeaponState::EWS_Dropped:
			OnWeaponStateDropped();
			break;
		default:
			break;
	}
}

void AWeapon::OnWeaponStateEquipped()
{
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
	EnableCustomDepth(false);	
	ClearRespawnTimer();

	AddOrRemoveHighPingDelegate(true);
}

void AWeapon::OnWeaponStateEquippedSecondary()
{
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
	
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
	WeaponMesh->MarkRenderStateDirty();
	
	ClearRespawnTimer();

	AddOrRemoveHighPingDelegate(false);
}

void AWeapon::OnWeaponStateDropped()
{
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

	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);
	StartRespawnOnDrop();

	AddOrRemoveHighPingDelegate(false);
}

void AWeapon::AddOrRemoveHighPingDelegate(bool bAddDelegate)
{
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if(BlasterOwnerCharacter && !BlasterOwnerCharacter->IsLocallyControlled() && bUseServerSideRewindDefault)
	{
		BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller) : BlasterOwnerController;

		if(BlasterOwnerController)
		{			
			if(bAddDelegate)
			{
				if(HasAuthority() && !BlasterOwnerController->HighPingDelegate.IsBound())
				{
					UE_LOG(LogTemp, Warning, TEXT("%s: High Ping Delegate Added"), *GetActorNameOrLabel());
					BlasterOwnerController->HighPingDelegate.AddDynamic(this, &AWeapon::AWeapon::OnPingTooHigh);
				}
			}
			else
			{				
				if(HasAuthority() && BlasterOwnerController->HighPingDelegate.IsBound())
				{
					UE_LOG(LogTemp, Warning, TEXT("%s: High Ping Delegate Removed"), *GetActorNameOrLabel());
					BlasterOwnerController->HighPingDelegate.RemoveDynamic(this, &AWeapon::AWeapon::OnPingTooHigh);
				}
			}
		}
	}
}

void AWeapon::StartRespawnOnDrop()
{
	if(HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(RespawnOnDropTimer, this, &AWeapon::RespawnWeapon, 30.0f);
	}
}

void AWeapon::RespawnWeapon()
{
	SetReplicateMovement(true);
	if(HasAuthority())
	{
		SetWeaponState(EWeaponState::EWS_Initial);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);

		if(GetOwner())
		{
			const FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
			WeaponMesh->DetachFromComponent(DetachRules);
			SetOwner(nullptr);
			BlasterOwnerCharacter = nullptr;
			BlasterOwnerController = nullptr;
		}
	
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		UE_LOG(LogTemp, Warning, TEXT("%s: Starting Location: %s, %s"), *GetActorNameOrLabel(), *StartingLocation.ToString(), *StartingRotation.ToString());
		SetActorLocationAndRotation(StartingLocation, StartingRotation, false, nullptr, ETeleportType::ResetPhysics);

		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
		WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

		Ammo = AmmoCapacity;
	}
}

void AWeapon::ClearRespawnTimer()
{
	if(HasAuthority())
	{
		GetWorld()->GetTimerManager().ClearTimer(RespawnOnDropTimer);
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

void AWeapon::Fire(const TArray<FVector_NetQuantize>& HitTargets)
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

void AWeapon::EnableCustomDepth(bool bInEnable)
{
	if(WeaponMesh)
	{
		WeaponMesh->SetRenderCustomDepth(bInEnable);
	}
}

bool AWeapon::IsEmpty() const
{
	return Ammo <= 0;
}

bool AWeapon::IsFull() const
{
	return Ammo == AmmoCapacity;
}

void AWeapon::GetSocketInformation(FTransform& OutSocketTransform, FVector& OutStart) const
{
	OutSocketTransform = FTransform::Identity;
	OutStart = FVector::ZeroVector;
	
	if(const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(MuzzleSocketFlashName))
	{
		OutSocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		OutStart = OutSocketTransform.GetLocation();		
	}
}

FVector AWeapon::TraceEndWithScatter(const FVector& HitTarget) const
{
	FTransform SocketTransform;
	FVector TraceStart;
	GetSocketInformation(SocketTransform, TraceStart);
	
	return TraceEndWithScatter(HitTarget, TraceStart);
}

FVector AWeapon::TraceEndWithScatter(const FVector& HitTarget, const FVector& TraceStart) const
{
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	const FVector RandomVector = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.0f, SphereRadius);
	const FVector EndLocation = SphereCenter + RandomVector;
	const FVector ToEndLocation = EndLocation - TraceStart;

	if(bShowDebugSpheres)
	{
		DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
		DrawDebugSphere(GetWorld(), EndLocation, 4.0f, 12, FColor::Orange, true);
		DrawDebugLine(GetWorld(), TraceStart, FVector(TraceStart + ToEndLocation * TRACE_LENGTH / ToEndLocation.Size()), FColor::Cyan, true);
	}
	
	return FVector(TraceStart + ToEndLocation * TRACE_LENGTH / ToEndLocation.Size());
}

void AWeapon::BurstTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& OutResult) const
{
	FTransform SocketTransform;
	FVector TraceStart;
	GetSocketInformation(SocketTransform, TraceStart);
	
	for(uint32 Shot = 0; Shot < NumberOfBurstShots; ++Shot)
	{
		OutResult.Add(TraceEndWithScatter(HitTarget, TraceStart));
	}
}

