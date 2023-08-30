// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"


class AProjectile;
class AWeapon;
class ABlasterPlayerController;
class ABlasterCharacter;

USTRUCT()
struct FCapsuleInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	float Radius;

	UPROPERTY()
	float HalfHeight;
};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	UPROPERTY()
	float Time;

	UPROPERTY()
	TMap<FName, FCapsuleInformation> HitBoxInfo;

	UPROPERTY()
	ABlasterCharacter* Character;
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bIsHit;

	UPROPERTY()
	bool bIsHeadShot;
};

USTRUCT(BlueprintType)
struct FShotgunServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<ABlasterCharacter*, uint32> HeadShots;

	UPROPERTY()
	TMap<ABlasterCharacter*, uint32> BodyShots;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULagCompensationComponent();
	friend ABlasterCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * @brief Debug Method for showing the frame history
	 * @param Package The package to draw
	 * @param Color Color of the debug boxes
	 */
	void ShowFramePackage(const FFramePackage& Package, const FColor& Color) const;

	/**
	 * @brief Applies damage to a character if server side rewind is successful. This is called when a client claims to
	 * have made a hit, but it did not go through on the server
	 * @param HitCharacter The Character being damaged
	 * @param TraceStart The start of the trace
	 * @param HitLocation The location on the hit character
	 * @param HitTime The time the client recorded a hit - Single Trip Time
	 */
	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime);

	/**
	 * @brief Applies damage to a character if server side rewind for projectiles is successful. This is called when a client claims to
	 * have made a hit, but it did not go through on the server
	 * @param HitCharacter The Character being damaged
	 * @param TraceStart The start of the trace
	 * @param InitialVelocity The initial velocity of the projectile
	 * @param HitTime The time the client recorded a hit - Single Trip Time
	 * @param DamageCauser The weapon that caused the damage (This is to fix an exploit where if the player switches weapons, it can cause nefarious damage)
	 */
	UFUNCTION(Server, Reliable)
	void ProjectileServerScoreRequest(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, const float HitTime, AProjectile* DamageCauser);
	
	/**
	 * @brief Rewinds for the shotgun and applies damage if a hit is successful. This is called when a client claims to have
	 * made a hit, but it did not go through on the server
	 * @param HitCharacters Characters being damaged
	 * @param TraceStart The start of the trace
	 * @param HitLocations The locations on the hit characters
	 * @param HitTime The time the client recorded a hit - Single Trip Time
	 * @return 
	 */
	UFUNCTION(Server, Reliable)
	void ShotgunServerScoreRequest(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime);
	
protected:
	virtual void BeginPlay() override;

	/**
	 * @brief Saves the current frame data for the owning character (Location, rotation, radius, half height) and puts it as part of the container
	 * @param Package The package to fill in with current owning character data
	 */
	void SaveFramePackage(FFramePackage& Package);

	/**
	 * @brief Interpolates the positions and rotations between two frame packages
	 * @param OlderFrame The older (less time passed) frame
	 * @param YoungerFrame  The younger (More time passed) frame
	 * @param HitTime The time the client recorded a hit - Single Trip Time
	 * @return An interpolated values frames between the Older and Younger Frame
	 * i.e. A Capsule will have their HalfHeight and Radius interpolated
	 */
	FFramePackage InterpolationBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime) const;

	/*
	 * HitScan Confirm Hit
	 */

	/**
	 * @brief Confirms the hit deduced by the server side rewind frame interpolation
	 * @param Package The exact match or interpolated frame to check
	 * @param HitCharacter The Character being damaged
	 * @param TraceStart The start of the trace
	 * @param HitLocation The location on the hit character
	 * @return Result indicating whether or not the server found a hit on the head of body
	 */
	FServerSideRewindResult ConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation);


	/*
	 * Projectile Confirm Hit
	 */

	/**
	 * @brief Confirms the hit deduced by the server side rewind frame interpolation for projectiles
	 * @param Package The exact match or interpolated frame to check
	 * @param HitCharacter The Character being damaged
	 * @param TraceStart The start of the trace
	 * @param InitialVelocity The initial velocity of the projectile
	 * @param HitTime The time the client recorded a hit - Single Trip Time
	 * @return Result indicating whether or not the server found a hit on the head of body
	 */
	FServerSideRewindResult ProjectileConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime);
	
	/*
	 * Shotgun Confirm Hit
	 */

	/**
	 * @brief Confirms the hit deduced by the server side rewind frame interpolation for shotguns
	 * @param FramePackages The frame packages of different characters to check
	 * @param TraceStart  The start of the trace
	 * @param HitLocations Each location the client recorded a hit
	 * @return A hit result with successful headshots and body shots
	 */
	FShotgunServerSideRewindResult ShotgunConfirmHit(const TArray<FFramePackage>& FramePackages, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations);

	/**
	 * @brief Caches the HitCharacters hit boxes for later use
	 * @param HitCharacter The Character being damaged
	 * @param OutFramePackage The cached frame package of the HitCharacter's hit boxes
	 */
	void CacheHitBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage);

	/**
	 * @brief Moves the hit characters hit boxes to the package information provided
	 * @param HitCharacter The Character being damaged
	 * @param Package Moves the hit characters hit boxes to the package information provided
	 */
	void MoveHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package);

	/**
	 * @brief Resets the hit characters hit boxes to the package provided. Additionally resets the collision of each
	 * hit box to No Collision
	 * @param HitCharacter The Character being damaged
	 * @param Package The package information to reset the hit character frame hit boxes to
	 */
	void ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package);

	/**
	 * @brief Enables or disables the collision hit boxes of the characters mesh
	 * @param HitCharacter The character being damaged
	 * @param InCollisionEnabled The collision type to set to
	 */
	void EnableCharacterMeshCollision(const ABlasterCharacter* HitCharacter, ECollisionEnabled::Type InCollisionEnabled);

	/**
	 * @brief Saves the frame history for the character owning this component
	 */
	void SaveFrameHistory();

	/**
	 * @brief Walks the linked list to find the FramePackage that best matches the passed in HitTime
	 * @param HitCharacter The character being hit
	 * @param HitTime The time the client recorded a hit - Single Trip Time
	 * @return A FramePackage that either exactly matches in item in the frame history, or one that has been interpolated
	 * between two moments in history
	 */
	FFramePackage GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime) const;

	/*
	 * HitScan Server Side Rewind
	 */
	/**
	 * @brief Performs server side rewind by rewinding the frame history, determining a frame to check, and performing a
	 * line trace to confirm the hit of the character
	 * @param HitCharacter The Character being damaged
	 * @param TraceStart The start of the trace
	 * @param HitLocation The location on the hit character
	 * @param HitTime The time the client recorded a hit - Single Trip Time
	 * @return Result indicating whether or not the server found a hit on the head of body
	 */
	FServerSideRewindResult ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime);

	/*
	 * Projectile Server Side Rewind
	 */
	/**
	 * @brief Performs server side rewind for projectiles by rewinding the frame history, determining a frame to check, and performing a
	 * line trace to confirm the hit of the character
	 * @param HitCharacter Character being damaged
	 * @param TraceStart The start of the trace
	 * @param InitialVelocity The initial velocity of the projectile
	 * @param HitTime The time the client recorded a hit - Single Trip Time
	 * @return Result indicating whether or not the server found a hit on the head of body
	 */
	FServerSideRewindResult ProjectileServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime);

	/*
	 * Shotgun Server Side Rewind
	 */
	/**
	 * @brief Performs server side rewind for the shotgun specifically by rewinding the frame history,
	 * determining a frame to check, and performing a line trace to confirm the hit of the character(s)
	 * @param HitCharacters Characters being damaged
	 * @param TraceStart The start of the trace
	 * @param HitLocations Each location the client recorded a hit
	 * @param HitTime The time the client recorded a hit - Single Trip Time
	 * @return A hit result with successful headshots and body shots
	 */
	FShotgunServerSideRewindResult ShotgunServerSideRewind(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime);

private:
	UPROPERTY()
	ABlasterCharacter* Character;

	UPROPERTY()
	ABlasterPlayerController* Controller;

	TDoubleLinkedList<FFramePackage> FrameHistory;

	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 4.0;
};
