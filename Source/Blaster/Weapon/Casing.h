// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casing.generated.h"

class USoundCue;

UCLASS()
class BLASTER_API ACasing : public AActor
{
	GENERATED_BODY()
	
public:	
	ACasing();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	virtual void DestroyOnHit();
private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* CasingMesh;

	UPROPERTY(EditAnywhere);
	float ShellEjectionImpulse{10.0f};

	UPROPERTY(EditAnywhere)
	USoundCue* ShellSound;

	bool bHitOnce{false};

	FTimerHandle DestroyOnHitTimerHandle;
	FTimerDelegate DestroyOnHitTimerDelegate;

	UPROPERTY(EditAnywhere)
	float DestroyOnHitWaitTime = 1.0f;
};
