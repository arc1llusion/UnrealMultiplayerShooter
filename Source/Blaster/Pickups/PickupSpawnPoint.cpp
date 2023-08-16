

#include "PickupSpawnPoint.h"

#include "Pickup.h"

APickupSpawnPoint::APickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	if(bSpawnImmediately)
	{
		if(HasAuthority())
		{
			SpawnPickup();
		}
	}
	else
	{
		StartSpawnPickupTimer(nullptr);	
	}	
}

void APickupSpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APickupSpawnPoint::SpawnPickup()
{
	if(PickupClasses.Num() == 0)
	{
		return;
	}

	SpawnedPickup = GetWorld()->SpawnActor<APickup>(PickupClasses[FMath::RandRange(0, PickupClasses.Num() - 1)], GetActorTransform());

	if(SpawnedPickup && HasAuthority())
	{
		SpawnedPickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartSpawnPickupTimer);
	}
}

void APickupSpawnPoint::SpawnPickupTimerFinished()
{
	if(HasAuthority())
	{
		SpawnPickup();
	}
}

void APickupSpawnPoint::StartSpawnPickupTimer(AActor* DestroyedActor)
{
	const float SpawnTime = FMath::FRandRange(MinimumSpawnTime, MaximumSpawnTime);

	GetWorldTimerManager().SetTimer(
		SpawnPickupTimer,
		this,
		&APickupSpawnPoint::SpawnPickupTimerFinished,
		SpawnTime
	);
}

