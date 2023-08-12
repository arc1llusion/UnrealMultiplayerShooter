#include "BuffComponent.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
	bHealing = true;

	HealingRate = HealAmount / HealingTime;
	AmountToHeal += HealAmount;
}

void UBuffComponent::HealRampUp(float DeltaTime)
{
	if(!bHealing || !Character || Character->IsEliminated())
	{
		return;
	}

	const float HealThisFrame = HealingRate * DeltaTime;
	Character->SetHealth(Character->GetHealth() + HealThisFrame); //Clamp is taken care of on Character->SetHealth
	Character->UpdateHUDHealth();
	
	AmountToHeal -= HealThisFrame;

	if(AmountToHeal <= 0.0f || Character->GetHealth() >= Character->GetMaxHealth())
	{
		bHealing = false;
		AmountToHeal = 0.0f;
	}
}

void UBuffComponent::SetInitialSpeeds(float InBaseSpeed, float InCrouchSpeed, float InAimWalkSpeed)
{
	InitialBaseSpeed = InBaseSpeed;
	InitialCrouchSpeed = InCrouchSpeed;
	InitialAimSpeed = InAimWalkSpeed;
}

void UBuffComponent::BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffAimSpeed, float BuffDuration)
{
	if(!Character)
	{
		return;
	}

	Character->GetWorldTimerManager().SetTimer(SpeedBuffTimer, this, &UBuffComponent::ResetSpeeds, BuffDuration);

	if(Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BuffBaseSpeed;
		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = BuffCrouchSpeed;
	}

	MulticastSpeedBuff(BuffBaseSpeed, BuffCrouchSpeed, BuffAimSpeed);
}

void UBuffComponent::ResetSpeeds()
{
	if(!Character || !Character->GetCharacterMovement())
	{
		return;
	}
	
	Character->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;

	MulticastSpeedBuff(InitialBaseSpeed, InitialCrouchSpeed, InitialAimSpeed);
}

void UBuffComponent::MulticastSpeedBuff_Implementation(float BaseSpeed, float CrouchSpeed, float AimSpeed)
{
	if(!Character || !Character->GetCharacterMovement())
	{
		return;
	}
	
	Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;

	if(Character->GetCombat())
	{
		Character->GetCombat()->SetMovementSpeed(BaseSpeed, AimSpeed);
	}
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	HealRampUp(DeltaTime);
}

