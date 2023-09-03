// Fill out your copyright notice in the Description page of Project Settings.


#include "FlagZone.h"

#include "Blaster/GameMode/CaptureTheFlagGameMode.h"
#include "Blaster/Weapon/Flag.h"
#include "Components/SphereComponent.h"

AFlagZone::AFlagZone()
{
	PrimaryActorTick.bCanEverTick = false;

	ZoneSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ZoneSphere"));
	SetRootComponent(ZoneSphere);
}

void AFlagZone::BeginPlay()
{
	Super::BeginPlay();

	ZoneSphere->OnComponentBeginOverlap.AddDynamic(this, &AFlagZone::OnSphereOverlap);
}

void AFlagZone::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(const auto OverlappingFlag = Cast<AFlag>(OtherActor))
	{
		if(OverlappingFlag->GetTeam() != Team)
		{
			if(const auto CaptureTheFlagGameMode = Cast<ACaptureTheFlagGameMode>(GetWorld()->GetAuthGameMode()))
			{
				UE_LOG(LogTemp, Warning, TEXT("Flag Captured"));
				
				CaptureTheFlagGameMode->FlagCaptured(OverlappingFlag, this);
			}

			OverlappingFlag->RespawnWeapon();
		}
	}
}

