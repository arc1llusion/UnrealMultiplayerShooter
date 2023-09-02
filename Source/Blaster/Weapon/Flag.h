// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Flag.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AFlag : public AWeapon
{
	GENERATED_BODY()

public:
	AFlag();

	virtual void Drop() override;

	virtual void OnWeaponStateEquipped() override;
	virtual void OnWeaponStateDropped() override;

private:

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* FlagMesh;
	
};
