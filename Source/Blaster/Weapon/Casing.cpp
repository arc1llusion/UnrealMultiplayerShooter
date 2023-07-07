// Fill out your copyright notice in the Description page of Project Settings.


#include "Casing.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundCue.h"

ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);

	// This makes it so the camera boom doesn't auto correct when in sight of the shell
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true);

	ShellEjectionImpulse = 10.0f;
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();

	// Get a random direction within a cone on the unit circle 
	const FVector RandomShell = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(GetActorForwardVector(), 20.0f);
	CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit);
	
	CasingMesh->AddImpulse(RandomShell * ShellEjectionImpulse);
}

void ACasing::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse, const FHitResult& Hit)
{
	// When the shell hits the ground once we'll do all the things, otherwise if it hits the ground again (and it will)
	// While simulating physics and bouncing, don't play the sounds again
	if(bHitOnce)
	{
		return;
	}
	
	bHitOnce = true;

	CasingMesh->OnComponentHit.RemoveDynamic(this, &ACasing::OnHit);
	
	if(ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}

	const UWorld* World = GetWorld();
	if(!World)
	{
		// If for some reason the world doesn't exist destroy it without binding to a timer
		Destroy();
		return;
	}
	
	DestroyOnHitTimerDelegate.BindUFunction(this, TEXT("DestroyOnHit"));

	FTimerManager &TimerManager = World->GetTimerManager();
	TimerManager.SetTimer(DestroyOnHitTimerHandle, DestroyOnHitTimerDelegate, DestroyOnHitWaitTime, false);
}

void ACasing::DestroyOnHit()
{
	// Clear the timer if we can
	if(const UWorld* World = GetWorld())
	{
		DestroyOnHitTimerDelegate.Unbind();
		if(DestroyOnHitTimerHandle.IsValid())
		{
			FTimerManager &TimerManager = World->GetTimerManager();
			TimerManager.ClearTimer(DestroyOnHitTimerHandle);
		}
	}
	
	Destroy();
}

