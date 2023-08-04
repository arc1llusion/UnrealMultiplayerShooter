// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Animation/AnimMontage.h"
#include "Blaster/Blaster.h"
#include "Blaster/GameMode/BlasterGameMode.h"
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
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	
	SimProxiesTurn();

	TimeSinceLastMovementReplication = 0;
}

void ABlasterCharacter::Eliminate()
{
	if(Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Drop();
	}
	
	MulticastEliminate();

	GetWorldTimerManager().SetTimer(EliminateHandle, this, &ABlasterCharacter::EliminateTimerFinished, EliminateDelay);
}

void ABlasterCharacter::MulticastEliminate_Implementation()
{
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
}

void ABlasterCharacter::EliminateTimerFinished()
{
	if(const auto BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>())
	{
		BlasterGameMode->RequestRespawn(this, Controller);
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

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();
	
	if(EliminationBotComponent)
	{
		EliminationBotComponent->DestroyComponent();
	}

	const ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	const bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress;
	
	if(Combat && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy();
	}
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	UpdateHUDHealth();

	if(HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);

		OnActorBeginOverlap.AddDynamic(this, &ABlasterCharacter::KillZOverlap);
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
		EnhancedInputComponent->BindAction(EquipActionAsset, ETriggerEvent::Triggered, this, &ABlasterCharacter::EquipAction);

		// Crouching
		EnhancedInputComponent->BindAction(CrouchInputAsset, ETriggerEvent::Started, this, &ABlasterCharacter::CrouchAction);

		// Aiming
		EnhancedInputComponent->BindAction(AimInputAsset, ETriggerEvent::Triggered, this, &ABlasterCharacter::AimAction);
		EnhancedInputComponent->BindAction(AimInputAsset, ETriggerEvent::Completed, this, &ABlasterCharacter::StopAimAction);

		//Firing Weapon
		EnhancedInputComponent->BindAction(FireInputAsset, ETriggerEvent::Started, this, &ABlasterCharacter::FireAction);
		EnhancedInputComponent->BindAction(FireInputAsset, ETriggerEvent::Completed, this, &ABlasterCharacter::StopFireAction);

		//Change Character in Lobby
		EnhancedInputComponent->BindAction(ChangeCharacterAsset, ETriggerEvent::Started, this, &ABlasterCharacter::ChangeCharacterAction);

		//Reload
		EnhancedInputComponent->BindAction(ReloadInputAsset, ETriggerEvent::Started, this, &ABlasterCharacter::ReloadAction);
	}
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if(Combat)
	{
		Combat->Character = this;
	}
}

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
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
				SectionName = FName(TEXT("Rifle"));
				break;
			case EWeaponType::EWT_Pistol:
				SectionName = FName(TEXT("Rifle"));
				break;
			default:
				SectionName = FName(TEXT("Rifle"));
				break;
		}

		AnimInstance->Montage_JumpToSection(SectionName);		

		FOnMontageEnded ReloadMontageEndDelegate;
		ReloadMontageEndDelegate.BindUObject(this, &ABlasterCharacter::OnReloadMontageEnd);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No Anim instance or reload weapon montage"));
	}
}

void ABlasterCharacter::OnReloadMontageEnd(UAnimMontage* AnimMontage, bool bInterrupted)
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
	Health = FMath::Clamp(Health - Damage, 0.0f, MaxHealth);

	UpdateHUDHealth();
	PlayHitReactMontage();

	if(Health <= 0.0f)
	{
		if(const auto BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>())
		{
			BlasterPlayerController = !BlasterPlayerController ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
			const auto AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
			BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
		}
	}
}

void ABlasterCharacter::KillZOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	if(Cast<AKillZVolume>(OtherActor))
	{
		if(const auto BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>())
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

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ABlasterCharacter::EquipAction(const FInputActionValue& Value)
{
	if(Combat)
	{
		if(HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquipButtonPressed();
		}
	}
}

void ABlasterCharacter::CrouchAction(const FInputActionValue& Value)
{
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
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::StopAimAction(const FInputActionValue& Value)
{
	if(Combat)
	{
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::FireAction(const FInputActionValue& Value)
{
	if(Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::StopFireAction(const FInputActionValue& Value)
{
	if(Combat)
	{
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

void ABlasterCharacter::ReloadAction(const FInputActionValue& Value)
{
	if(Combat)
	{
		Combat->Reload();
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if(Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
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
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if(Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ABlasterCharacter::OnRep_Health()
{
	UpdateHUDHealth();

	if(!bEliminated)
	{
		PlayHitReactMontage();
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
	return Combat && Combat->bAiming;
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
