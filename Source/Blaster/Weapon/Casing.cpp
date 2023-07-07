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

	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true);

	ShellEjectionImpulse = 10.0f;
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();

	const FVector RandomShell = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(GetActorForwardVector(), 20.0f);
	CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit);
	
	CasingMesh->AddImpulse(RandomShell * ShellEjectionImpulse);
}

void ACasing::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if(bHitOnce)
	{
		return;
	}
	
	bHitOnce = true;
	
	if(ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}

	const UWorld* World = GetWorld();
	if(!World)
	{
		return;
	}
	
	DestroyOnHitTimerDelegate.BindUFunction(this, TEXT("DestroyOnHit"));

	FTimerManager &TimerManager = World->GetTimerManager();
	TimerManager.SetTimer(DestroyOnHitTimerHandle, DestroyOnHitTimerDelegate, DestroyOnHitWaitTime, false);
}

void ACasing::DestroyOnHit()
{
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

