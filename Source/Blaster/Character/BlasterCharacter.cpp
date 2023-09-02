// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Animation/AnimMontage.h"
#include "Blaster/Blaster.h"
#include "Blaster/BlasterComponents/BuffComponent.h"
#include "Blaster/BlasterComponents/LagCompensationComponent.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Blaster/HUD/OverheadWidget.h"
#include "Components/SphereComponent.h"
#include "GameFramework/KillZVolume.h"

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadCollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverheadCollision"));
	OverheadCollisionSphere->SetupAttachment(RootComponent);
	OverheadCollisionSphere->SetCollisionObjectType(ECC_Pawn);
	OverheadCollisionSphere->SetGenerateOverlapEvents(true);
	FCollisionResponseContainer SphereCollisionResponses;
	SphereCollisionResponses.SetAllChannels(ECollisionResponse::ECR_Ignore);
	SphereCollisionResponses.SetResponse(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	OverheadCollisionSphere->SetCollisionResponseToChannels(SphereCollisionResponses);

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensation"));	

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 850.0f, 0.0f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;

	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttachedGrenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), GrenadeSocketName);
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
	DOREPLIFETIME(ABlasterCharacter, Shield);
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	SpawnDefaultWeapon();

	UpdateHUDReadyOverlay();
	UpdateHUDAmmo();
	UpdateHUDHealth();
	UpdateHUDShield();
	UpdateHUDTeamScores();

	if(HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
		OnActorBeginOverlap.AddDynamic(this, &ABlasterCharacter::KillZOverlap);
	}

	if(AttachedGrenade)
	{
		AttachedGrenade->SetVisibility(false);
	}

	CreateCapsuleHitBoxes();
}

void ABlasterCharacter::CreateCapsuleHitBoxes()
{
	UE_LOG(LogTemp, Warning, TEXT("Blaster Character BeginPlay"));
	
	if(GetMesh())
	{
		HitCollisionCapsules.Empty();
		
		for (TObjectPtr<USkeletalBodySetup> SkeletalBodySetup : GetMesh()->GetPhysicsAsset()->SkeletalBodySetups)
		{
			FName BoneName = SkeletalBodySetup->BoneName;
			FTransform BoneWorldTransform = GetMesh()->GetBoneTransform(GetMesh()->GetBoneIndex(BoneName));
		
			for (FKSphylElem Capsule : SkeletalBodySetup->AggGeom.SphylElems)
			{
				FTransform LocTransform = Capsule.GetTransform();
				FTransform WorldTransform = LocTransform * BoneWorldTransform;

				auto HitCapsule = NewObject<UCapsuleComponent>(this, UCapsuleComponent::StaticClass(), BoneName, RF_Transient);
				HitCapsule->SetupAttachment(GetMesh(), BoneName);
				HitCapsule->SetWorldTransform(WorldTransform);
				HitCapsule->SetHiddenInGame(!bDrawHitBoxes);

				//Set the radius first, because capsule half height takes the maximum between 0, radius, and half height
				//By default the radius is possibly 22, so if you set a lower half height it'll set the half height to 22
				HitCapsule->SetCapsuleRadius(Capsule.Radius, false);
				HitCapsule->SetCapsuleHalfHeight(Capsule.Length / 2 + Capsule.Radius, false);

				HitCapsule->SetCollisionObjectType(ECC_HitBox);
				HitCapsule->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
				HitCapsule->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
				HitCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);

				HitCapsule->RegisterComponent();

				HitCollisionCapsules.Add(BoneName, HitCapsule);
			}
		}
	}
}

void ABlasterCharacter::DrawDebugHitBoxes()
{
	if(bDrawHitBoxes)
	{
		for(const auto& Capsule : HitCollisionCapsules)
		{
			DrawDebugCapsule(
				GetWorld(),
				Capsule.Value->GetComponentLocation(),
				Capsule.Value->GetScaledCapsuleHalfHeight(),
				Capsule.Value->GetScaledCapsuleRadius(),
				Capsule.Value->GetComponentRotation().Quaternion(),
				FColor::Red);
		}
	}
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	
	SimProxiesTurn();

	TimeSinceLastMovementReplication = 0;
}

void ABlasterCharacter::SetTeamColor(ETeam Team)
{
	if(GetMesh() == nullptr)
	{
		return;
	}
	
	switch(Team)
	{
		case ETeam::ET_NoTeam:
			GetMesh()->SetRenderCustomDepth(false);
			DissolveMaterialInstance = BlueDissolveMaterialInstance;
			break;

		case ETeam::ET_BlueTeam:
			GetMesh()->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
			GetMesh()->MarkRenderStateDirty();
			GetMesh()->SetRenderCustomDepth(true);
			DissolveMaterialInstance = BlueDissolveMaterialInstance;
			break;

		case ETeam::ET_RedTeam:
			GetMesh()->SetCustomDepthStencilValue(CUSTOM_DEPTH_RED);
			GetMesh()->MarkRenderStateDirty();
			GetMesh()->SetRenderCustomDepth(true);
			DissolveMaterialInstance = RedDissolveMaterialInstance;
			break;

		default:
			GetMesh()->SetRenderCustomDepth(false);
			DissolveMaterialInstance = BlueDissolveMaterialInstance;
			break;
	}
}

ETeam ABlasterCharacter::GetTeam()
{
	BlasterPlayerState = BlasterPlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : BlasterPlayerState;
	if(BlasterPlayerState == nullptr)
	{
		return ETeam::ET_NoTeam;
	}

	return BlasterPlayerState->GetTeam();
}

void ABlasterCharacter::Eliminate(const bool bInLeftGame)
{
	if(Combat)
	{
		HandleWeaponOnElimination(Combat->EquippedWeapon);
		HandleWeaponOnElimination(Combat->SecondaryWeapon);
		HandleWeaponOnElimination(Combat->EquippedFlag);
	}
	
	MulticastEliminate(bInLeftGame);
}

void ABlasterCharacter::MulticastEliminate_Implementation(const bool bInLeftGame)
{
	bLeftGame = bInLeftGame;
	
	if(BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(0);
	}	
	
	bEliminated = true;
	PlayEliminationMontage();

	StartDissolve();

	StopCharacterMovement();
	DisableCollision();

	SpawnEliminationBot();

	HideAimingScope();

	if(CrownComponent)
	{
		CrownComponent->DestroyComponent();
		CrownComponent = nullptr;
	}
	
	GetWorldTimerManager().SetTimer(EliminateHandle, this, &ABlasterCharacter::EliminateTimerFinished, EliminateDelay);
}

void ABlasterCharacter::EliminateTimerFinished()
{
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	if(BlasterGameMode)
	{
		if(!bLeftGame)
		{
			BlasterGameMode->RequestRespawn(this, Controller);
		}
	}
	
	if(bLeftGame && IsLocallyControlled())
	{
		OnLeftGame.Broadcast();
	}
}

void ABlasterCharacter::StopCharacterMovement()
{
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	if(BlasterPlayerController)
	{
		DisableInput(BlasterPlayerController);
	}
}

void ABlasterCharacter::DisableCollision()
{
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABlasterCharacter::SpawnEliminationBot()
{
	const FVector EliminationBotSpawnPoint{GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.0f};
	EliminationBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
		this,
		EliminationBotEffect,
		EliminationBotSpawnPoint,
		GetActorRotation());

	if(EliminationBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			EliminationBotSound,
			EliminationBotSpawnPoint);
	}
}

void ABlasterCharacter::HandleWeaponOnElimination(AWeapon* Weapon)
{
	if(Weapon)
	{
		if(Weapon->IsDefaultWeapon())
		{
			Weapon->Destroy();
		}
		else
		{
			Weapon->Drop();
		}
	}
}

void ABlasterCharacter::ServerLeaveGame_Implementation()
{
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	if(BlasterGameMode)
	{
		BlasterPlayerState = BlasterPlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : BlasterPlayerState;
		BlasterGameMode->PlayerLeftGame(BlasterPlayerState);
	}
}

void ABlasterCharacter::HideAimingScope() const
{
	if(IsLocallyControlled() && Combat)
	{
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::MulticastGainedTheLead_Implementation()
{
	if(CrownSystem == nullptr)
	{
		return;
	}

	if(CrownComponent == nullptr)
	{
		CrownComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
							CrownSystem,
							GetMesh(),
							NAME_None,
							GetActorLocation() + FVector(0.0f, 0.0f, 110.0f),
							GetActorRotation(),
							EAttachLocation::KeepWorldPosition,
							false);
	}

	if(CrownComponent != nullptr)
	{
		CrownComponent->Activate();
	}
}

void ABlasterCharacter::MulticastLostTheLead_Implementation()
{
	if(CrownComponent != nullptr)
	{
		CrownComponent->DestroyComponent();
		CrownComponent = nullptr;
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RotateInPlace(DeltaTime);	
	HideCharacterIfCameraClose();
	PollInit();
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent *EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpActionAsset, ETriggerEvent::Triggered, this, &ABlasterCharacter::Jump);
		EnhancedInputComponent->BindAction(JumpActionAsset, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveActionAsset, ETriggerEvent::Triggered, this, &ABlasterCharacter::MoveAction);

		// Looking
		EnhancedInputComponent->BindAction(LookActionAsset, ETriggerEvent::Triggered, this, &ABlasterCharacter::LookAction);

		// Equip
		EnhancedInputComponent->BindAction(EquipActionAsset, ETriggerEvent::Started, this, &ABlasterCharacter::EquipAction);

		// Crouching
		EnhancedInputComponent->BindAction(CrouchInputAsset, ETriggerEvent::Started, this, &ABlasterCharacter::CrouchAction);

		// Aiming
		EnhancedInputComponent->BindAction(AimInputAsset, ETriggerEvent::Started, this, &ABlasterCharacter::AimAction);
		EnhancedInputComponent->BindAction(AimInputAsset, ETriggerEvent::Completed, this, &ABlasterCharacter::StopAimAction);

		//Firing Weapon
		EnhancedInputComponent->BindAction(FireInputAsset, ETriggerEvent::Started, this, &ABlasterCharacter::FireAction);
		EnhancedInputComponent->BindAction(FireInputAsset, ETriggerEvent::Completed, this, &ABlasterCharacter::StopFireAction);

		//Change Character in Lobby
		EnhancedInputComponent->BindAction(ChangeCharacterAsset, ETriggerEvent::Started, this, &ABlasterCharacter::ChangeCharacterAction);

		//Set ready status in lobby
		EnhancedInputComponent->BindAction(ReadyInputAsset, ETriggerEvent::Started, this, &ABlasterCharacter::ReadyAction);

		//Reload
		EnhancedInputComponent->BindAction(ReloadInputAsset, ETriggerEvent::Started, this, &ABlasterCharacter::ReloadAction);

		//Throw Grenade
		EnhancedInputComponent->BindAction(ThrowGrenadeInputAsset, ETriggerEvent::Started, this, &ABlasterCharacter::ThrowGrenadeAction);
	}
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if(Combat)
	{
		Combat->Character = this;
	}

	if(Buff && Combat)
	{
		Buff->Character = this;
		Buff->SetInitialSpeeds(GetCharacterMovement()->MaxWalkSpeed, GetCharacterMovement()->MaxWalkSpeedCrouched, Combat->AimWalkSpeed);
		Buff->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity);
	}

	if(LagCompensation)
	{
		LagCompensation->Character = this;
		if(Controller)
		{
			LagCompensation->Controller = Cast<ABlasterPlayerController>(Controller);
		}
	}
}

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();
	
	if(EliminationBotComponent)
	{
		EliminationBotComponent->DestroyComponent();
	}

	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	const bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress;
	
	if(Combat && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy();
	}
}

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
	if(Combat && Combat->bIsHoldingTheFlag)
	{
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	
	if(GetDisableGameplayFromController())
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	
	if(GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if(TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}

		CalculateAimOffsetPitch();
	}
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if(!Combat || !Combat->EquippedWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Combat or Equipped Weapon"));
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if(AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		const FName SectionName = bAiming ? FName(TEXT("RifleAim")) : FName(TEXT("RifleHip"));

		AnimInstance->Montage_JumpToSection(SectionName);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No Anim instance or fire weapon montage"));
	}
}

void ABlasterCharacter::PlayReloadMontage()
{
	if(!Combat || !Combat->EquippedWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Combat or Equipped Weapon"));
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if(AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		
		FName SectionName;
		switch(Combat->EquippedWeapon->GetWeaponType())
		{
			case EWeaponType::EWT_AssaultRifle:
				SectionName = FName(TEXT("Rifle"));
				break;
			case EWeaponType::EWT_RocketLauncher:
				SectionName = FName(TEXT("RocketLauncher"));
				break;
			case EWeaponType::EWT_Pistol:
				SectionName = FName(TEXT("Pistol"));
				break;
			case EWeaponType::EWT_SubMachineGun:
				SectionName = FName(TEXT("Pistol"));
				break;
			case EWeaponType::EWT_Shotgun:
				SectionName = FName(TEXT("Shotgun"));
				break;
			case EWeaponType::EWT_SniperRifle:
				SectionName = FName(TEXT("SniperRifle"));
				break;
			case EWeaponType::EWT_GrenadeLauncher:
				SectionName = FName(TEXT("GrenadeLauncher"));
				break;
			default:
				SectionName = FName(TEXT("Rifle"));
				break;
		}

		AnimInstance->Montage_JumpToSection(SectionName);		

		FOnMontageEnded ReloadMontageEndDelegate;
		ReloadMontageEndDelegate.BindUObject(this, &ABlasterCharacter::OnReloadMontageEnd);

		AnimInstance->Montage_SetEndDelegate(ReloadMontageEndDelegate, ReloadMontage);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No Anim instance or reload weapon montage"));
	}
}

void ABlasterCharacter::JumpToReloadMontageSection(const FName& SectionName) const
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if(AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::OnReloadMontageEnd(UAnimMontage* AnimMontage, bool bInterrupted) const
{
	if(!Combat)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Reload Montage End"));
	
	if(bInterrupted)
	{
		Combat->CombatState = ECombatState::ECS_Unoccupied;
	}
	else
	{
		Combat->FinishReloading();
	}
}

void ABlasterCharacter::PlayEliminationMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if(AnimInstance && EliminationMontage)
	{
		AnimInstance->Montage_Play(EliminationMontage);
	}
}

void ABlasterCharacter::PlayThrowGrenadeMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if(AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);

		FOnMontageEnded ThrowGrenadeMontageEndDelegate;
		ThrowGrenadeMontageEndDelegate.BindUObject(this, &ABlasterCharacter::OnThrowGrenadeMontageEnd);

		AnimInstance->Montage_SetEndDelegate(ThrowGrenadeMontageEndDelegate, ThrowGrenadeMontage);
	}
}

void ABlasterCharacter::OnThrowGrenadeMontageEnd(UAnimMontage* AnimMontage, bool bInterrupted) const
{
	if(!Combat)
	{
		return;
	}

	const FString Interrupted = bInterrupted ? TEXT("True") : TEXT("False");
	UE_LOG(LogTemp, Warning, TEXT("Throw Grenade Montage End %s"), *Interrupted);
	
	if(bInterrupted)
	{
		Combat->ThrowGrenadeFinished();
	}
	else
	{
		Combat->ThrowGrenadeFinished();
	}
}

void ABlasterCharacter::PlaySwapMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if(AnimInstance && SwapMontage)
	{
		AnimInstance->Montage_Play(SwapMontage);

		FOnMontageEnded SwapMontageEndDelegate;
		SwapMontageEndDelegate.BindUObject(this, &ABlasterCharacter::OnSwapMontageEnd);

		AnimInstance->Montage_SetEndDelegate(SwapMontageEndDelegate, SwapMontage);
	}
}

void ABlasterCharacter::OnSwapMontageEnd(UAnimMontage* AnimMontage, bool bInterrupted) const
{
	if(!Combat)
	{
		return;
	}

	const FString Interrupted = bInterrupted ? TEXT("True") : TEXT("False");
	UE_LOG(LogTemp, Warning, TEXT("%s: Swap Montage End %s"), *GetActorNameOrLabel(),  *Interrupted);
	
	if(Combat)
	{
		Combat->FinishSwap();
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if(!Combat || !Combat->EquippedWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("Hit React Montage - No Combat or Equipped Weapon"));
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if(AnimInstance && HitReactMontage && !AnimInstance->IsAnyMontagePlaying())
	{
		AnimInstance->Montage_Play(HitReactMontage);
		const FName SectionName = FName(TEXT("FromFront"));

		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatorController, AActor* DamageCauser)
{
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;

	if(bEliminated || BlasterGameMode == nullptr)
	{
		return;
	}

	Damage = BlasterGameMode->CalculateDamage(InstigatorController, Controller, Damage);

	if(Damage <= 0)
	{
		return;
	}

	float DamageToHealth = Damage;

	if(Shield > 0.0f)
	{
		if(Shield >= Damage)
		{
			Shield = FMath::Clamp(Shield - Damage, 0.0f, MaxShield);
			DamageToHealth = 0.0f;
		}
		else
		{
			DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0.0f, Damage);
			Shield = 0.0f;			
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Damage to Health: %f"), DamageToHealth);
	
	Health = FMath::Clamp(Health - DamageToHealth, 0.0f, MaxHealth);

	UpdateHUDHealth();
	UpdateHUDShield();
	
	PlayHitReactMontage();

	if(Health <= 0.0f)
	{
		BlasterPlayerController = !BlasterPlayerController ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
		const auto AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
		BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
	}
}

void ABlasterCharacter::KillZOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	if(Cast<AKillZVolume>(OtherActor))
	{
		BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
		if(BlasterGameMode)
		{
			BlasterPlayerController = !BlasterPlayerController ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
			BlasterGameMode->PlayerFell(this, BlasterPlayerController);
		}
	}
}

void ABlasterCharacter::PollInit()
{
	if(!BlasterPlayerState)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();

		if(BlasterPlayerState)
		{
			BlasterPlayerState->AddToScore(0.0f);
			BlasterPlayerState->AddToDefeats(0);
			SetTeamColor(BlasterPlayerState->GetTeam());

			if(const auto BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this)))
			{
				TArray<ABlasterPlayerState*> TopScoringPlayers;
				BlasterGameState->GetTopScoringPlayers(TopScoringPlayers);
				if(TopScoringPlayers.Contains(BlasterPlayerState))
				{
					MulticastGainedTheLead();
				}
			}
		}
	}
}

void ABlasterCharacter::MoveAction(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector ForwardDirection(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(ForwardDirection, MovementVector.Y);

		const FVector RightDirection(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ABlasterCharacter::LookAction(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	//IsAiming checks for Combat
	const float AimScaleFactor = IsAiming() ? Combat->GetWeaponAimScaleFactor() : 1.0f;	

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X * AimScaleFactor);
		AddControllerPitchInput(LookAxisVector.Y * AimScaleFactor);
	}
}

void ABlasterCharacter::EquipAction(const FInputActionValue& Value)
{
	if(Combat)
	{
		if(Combat->bIsHoldingTheFlag)
		{
			return;
		}
		
		if(Combat->CombatState == ECombatState::ECS_Unoccupied)
		{
			ServerEquipButtonPressed();
		}

		if(Combat->ShouldSwapWeapons() &&
		   !HasAuthority() &&
		   Combat->CombatState == ECombatState::ECS_Unoccupied &&
		   OverlappingWeapon == nullptr)
		{
			PlaySwapMontage();
			Combat->CombatState = ECombatState::ECS_SwappingWeapons;
			bIsFinishedSwapping = false;
		}
	}
}

void ABlasterCharacter::CrouchAction(const FInputActionValue& Value)
{
	if(Combat && Combat->bIsHoldingTheFlag)
	{
		return;
	}
	
	if(bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABlasterCharacter::AimAction(const FInputActionValue& Value)
{
	if(Combat)
	{
		if(Combat->bIsHoldingTheFlag)
		{
			return;
		}
		
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::StopAimAction(const FInputActionValue& Value)
{
	if(Combat)
	{
		if(Combat->bIsHoldingTheFlag)
		{
			return;
		}
		
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::FireAction(const FInputActionValue& Value)
{
	if(Combat)
	{
		if(Combat->bIsHoldingTheFlag)
		{
			return;
		}
		
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::StopFireAction(const FInputActionValue& Value)
{
	if(Combat)
	{
		if(Combat->bIsHoldingTheFlag)
		{
			return;
		}
		
		Combat->FireButtonPressed(false);
	}
}

void ABlasterCharacter::ChangeCharacterAction(const FInputActionValue& Value)
{
	const float Axis = Value.Get<float>();
	if(Axis > 0)
	{
		BlasterPlayerController = !BlasterPlayerController ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController; 
		if(BlasterPlayerController)
		{
			BlasterPlayerController->ForwardDesiredPawn();
		}
	}
	else if(Axis < 0)
	{
		BlasterPlayerController = !BlasterPlayerController ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController; 
		if(BlasterPlayerController)
		{
			BlasterPlayerController->BackDesiredPawn();
		}
	}
}

void ABlasterCharacter::ReadyAction(const FInputActionValue& Value)
{
	BlasterPlayerController = !BlasterPlayerController ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController; 
	if(BlasterPlayerController)
	{
		BlasterPlayerController->SetReady();
	}
}

void ABlasterCharacter::ReloadAction(const FInputActionValue& Value)
{
	if(Combat)
	{
		if(Combat->bIsHoldingTheFlag)
		{
			return;
		}
		
		Combat->Reload();
	}
}

void ABlasterCharacter::ThrowGrenadeAction(const FInputActionValue& Value)
{
	if(Combat)
	{
		if(Combat->bIsHoldingTheFlag)
		{
			return;
		}
		
		Combat->ThrowGrenade();
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if(Combat)
	{
		if(OverlappingWeapon)
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else if (Combat->ShouldSwapWeapons())
		{
			Combat->SwapWeapons();
		}
	}
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if(Combat && Combat->EquippedWeapon == nullptr)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		StartingAimRotation = FRotator(0.0f, GetBaseAimRotation().Yaw, 0.0f);
		return;
	}
	
	CalculateAimOffsetYaw(DeltaTime);
	CalculateAimOffsetPitch();
}

float ABlasterCharacter::CalculateSpeed() const
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.0f;
	return Velocity.Size();
}

void ABlasterCharacter::CalculateAimOffsetYaw(float DeltaTime)
{
	const float Speed = CalculateSpeed();
	
	const bool bIsInAir = GetCharacterMovement()->IsFalling();

	if(Speed == 0.0f && !bIsInAir) // Standing still, not jumping
	{
		bRotateRootBone = true;
		
		const FRotator CurrentAimRotation = FRotator(0.0f, GetBaseAimRotation().Yaw, 0.0f);
		const FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AimOffsetYaw = DeltaAimRotation.Yaw;

		if(TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			AimOffsetInterpolationYaw = AimOffsetYaw;
		}
		
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}
	
	if(Speed > 0.0f|| bIsInAir) //Running or jumping
	{
		bRotateRootBone = false;
		
		StartingAimRotation = FRotator(0.0f, GetBaseAimRotation().Yaw, 0.0f);
		AimOffsetYaw = 0;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}
}

void ABlasterCharacter::CalculateAimOffsetPitch()
{
	AimOffsetPitch = GetBaseAimRotation().Pitch;
	if(AimOffsetPitch > 90.0f && !IsLocallyControlled()) 
	{
		//Map pitch from [270, 360) and [-90, 0)
		const FVector2D InRange(270.0f, 360.0f);
		const FVector2D OutRange(-90.0f, 0.0f);
		AimOffsetPitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AimOffsetPitch);
	}
}

void ABlasterCharacter::SimProxiesTurn()
{
	if(!Combat || !Combat->EquippedWeapon)
	{
		return;
	}
	
	bRotateRootBone = false;

	const float Speed = CalculateSpeed();
	if(Speed > 0.0f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if(FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if(ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::Jump()
{
	if(Combat && Combat->bIsHoldingTheFlag)
	{
		return;
	}
	
	if(bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if(AimOffsetYaw > 90.0f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
		
	}
	else if(AimOffsetYaw < -90.0f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	if(TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		AimOffsetInterpolationYaw = FMath::FInterpTo(AimOffsetInterpolationYaw, 0.0f, DeltaTime, 4.0f);
		AimOffsetYaw = AimOffsetInterpolationYaw;

		if(FMath::Abs(AimOffsetYaw) < 15.0f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.0f, GetBaseAimRotation().Yaw, 0.0f);
		}
	}
}

void ABlasterCharacter::HideCharacterIfCameraClose()
{
	if(!IsLocallyControlled())
	{
		return;
	}

	if((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if(Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}

		if(Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		{
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if(Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}

		if(Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		{
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ABlasterCharacter::OnRep_Health(float LastHealth)
{
	UpdateHUDHealth();

	if(Health < LastHealth)
	{
		if(!bEliminated)
		{
			PlayHitReactMontage();
		}
	}
}

void ABlasterCharacter::OnRep_Shield(float LastShield)
{
	UpdateHUDShield();

	if(Shield < LastShield)
	{
		if(!bEliminated)
		{
			PlayHitReactMontage();
		}
	}
}

void ABlasterCharacter::UpdateHUDReadyOverlay()
{
	BlasterPlayerController = !BlasterPlayerController ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if(BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDPlayersReady();
	}
}

void ABlasterCharacter::UpdateHUDHealth()
{
	BlasterPlayerController = !BlasterPlayerController ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if(BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}

	if(OverheadWidget)
	{
		if(const auto Widget = Cast<UOverheadWidget>(OverheadWidget->GetWidget()))
		{
			Widget->UpdateOverlayHealth(Health, MaxHealth);
		}
	}
}

void ABlasterCharacter::UpdateHUDShield()
{
	BlasterPlayerController = !BlasterPlayerController ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if(BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDShield(Shield, MaxShield);
	}

	if(OverheadWidget)
	{
		if(const auto Widget = Cast<UOverheadWidget>(OverheadWidget->GetWidget()))
		{
			Widget->UpdateOverlayShield(Shield, MaxShield);
		}
	}
}

void ABlasterCharacter::UpdateHUDAmmo()
{
	BlasterPlayerController = !BlasterPlayerController ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if(BlasterPlayerController && Combat && Combat->EquippedWeapon)
	{
		BlasterPlayerController->SetHUDCarriedAmmo(Combat->CarriedAmmo);
		BlasterPlayerController->SetHUDWeaponAmmo(Combat->EquippedWeapon->GetAmmo());
	}
}

void ABlasterCharacter::UpdateHUDTeamScores()
{
	BlasterPlayerController = !BlasterPlayerController ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;

	if(BlasterPlayerController)
	{
		BlasterGameMode = !BlasterGameMode ? Cast<ABlasterGameMode>(GetWorld()->GetAuthGameMode()) : BlasterGameMode;
		if(BlasterGameMode)
		{			
			if(BlasterGameMode->IsTeamsMatch())
			{
				BlasterPlayerController->InitializeTeamScores();
			}
			else
			{
				BlasterPlayerController->HideTeamScores();
			}
		}
	}
}

void ABlasterCharacter::SpawnDefaultWeapon()
{
	UWorld* World = GetWorld();
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	if(World && !bEliminated && DefaultWeaponClass && BlasterGameMode)
	{
		AWeapon* StartingWeapon = World->SpawnActor<AWeapon>(DefaultWeaponClass);
		StartingWeapon->SetIsDefaultWeapon(true);
		
		if(Combat)
		{
			Combat->EquipWeapon(StartingWeapon);
		}
	}
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if(DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(DissolveParameterName, DissolveValue);
	}
}

void ABlasterCharacter::StartDissolve()
{
	if(DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);

		DynamicDissolveMaterialInstance->SetScalarParameterValue(DissolveParameterName, 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(GlowParameterName, 200.0f);
	}
	
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);
	if(DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

bool ABlasterCharacter::GetDisableGameplayFromController()
{
	BlasterPlayerController = !BlasterPlayerController ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if(BlasterPlayerController)
	{
		return BlasterPlayerController->IsGameplayDisabled();
	}

	return false;
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if(IsLocallyControlled())
	{
		if(OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(false);			
		}
	}
	
	OverlappingWeapon = Weapon;
	
	if(IsLocallyControlled())
	{
		if(OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if(OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}

	if(LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

bool ABlasterCharacter::IsWeaponEquipped() const
{
	return Combat && Combat->EquippedWeapon;
}

bool ABlasterCharacter::IsAiming() const
{
	return Combat && Combat->bIsAiming;
}

AWeapon* ABlasterCharacter::GetEquippedWeapon() const
{
	if(!Combat)
	{
		return nullptr;
	}

	return Combat->EquippedWeapon;
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if(!Combat)
	{
		return FVector::Zero();
	}

	return Combat->HitTarget;
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	if(!Combat)
	{
		return ECombatState::ECS_MAX;
	}

	return Combat->CombatState;
}

bool ABlasterCharacter::IsLocallyReloading() const
{
	if(!Combat)
	{
		return false;
	}

	return Combat->bLocallyReloading;
}

bool ABlasterCharacter::IsHoldingTheFlag() const
{
	if(!Combat)
	{
		return false;
	}

	return Combat->bIsHoldingTheFlag;
}


// ReSharper disable once CppMemberFunctionMayBeConst
void ABlasterCharacter::OnOverheadOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                               UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(const auto BlasterActor = Cast<ABlasterCharacter>(OtherActor))
	{
		const auto ThisSphereComponent = Cast<USphereComponent>(OverlappedComponent);
		const auto OtherSphereComponent = Cast<USphereComponent>(OtherComp);
		if(ThisSphereComponent && OtherSphereComponent)
		{
			BlasterActor->SetDisplayOverheadWidget(true);
		}
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void ABlasterCharacter::OnOverheadOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if(const auto BlasterActor = Cast<ABlasterCharacter>(OtherActor))
	{
		const auto ThisSphereComponent = Cast<USphereComponent>(OverlappedComponent);
		const auto OtherSphereComponent = Cast<USphereComponent>(OtherComp);
		if(ThisSphereComponent && OtherSphereComponent)
		{
			BlasterActor->SetDisplayOverheadWidget(false);
		}
	}
}

void ABlasterCharacter::SetupOverheadOverlapEvents()
{
	if(IsLocallyControlled())
	{
		OverheadCollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ABlasterCharacter::OnOverheadOverlapBegin);
		OverheadCollisionSphere->OnComponentEndOverlap.AddDynamic(this, &ABlasterCharacter::OnOverheadOverlapEnd);
		if(OverheadWidget)
		{
			OverheadWidget->SetVisibility(true);
		}
	}
	else
	{
		if(OverheadWidget)
		{
			OverheadWidget->SetVisibility(false);
		}
	}
}

void ABlasterCharacter::SetDisplayOverheadWidget(bool bInDisplay) const
{
	if(!OverheadWidget)
	{
		return;
	}

	OverheadWidget->SetVisibility(IsLocallyControlled() ? true : bInDisplay);
}