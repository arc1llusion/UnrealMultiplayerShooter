#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBuffComponent();
	friend class ABlasterCharacter;

	void Heal(float HealAmount, float HealingTime);
	
	void BuffSpeed(float BuffBaseSPeed, float BuffCrouchSpeed, float BuffDuration);
	void SetInitialSpeeds(float InBaseSpeed, float InCrouchSpeed);

protected:
	virtual void BeginPlay() override;
	
	void HealRampUp(float DeltaTime);

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	ABlasterCharacter* Character;

	/*
	 * Health Buff
	 */
	bool bHealing = false;
	float HealingRate = 0.0f;
	float AmountToHeal = 0.0f;

	/*
	 * Speed Buff
	 */

	FTimerHandle SpeedBuffTimer;
	void ResetSpeeds();

	float InitialBaseSpeed;
	float InitialCrouchSpeed;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);
};
