// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "OverheadWidget.generated.h"


class UTextBlock;
class UProgressBar;

/**
 * 
 */
UCLASS()
class BLASTER_API UOverheadWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* DisplayText;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* DisplayHealth;

	void SetDisplayText(const FString& TextToDisplay) const;
	
	UFUNCTION(BlueprintCallable)
	void ShowPlayerRemoteNetRole(APawn* InPawn);

	UFUNCTION(BlueprintCallable)
	void ShowPlayerLocalNetRole(APawn* InPawn);
	void BindAndTickPlayerStateTimer(FTimerManager& TimerManager, APawn* InPawn);

	UFUNCTION(BlueprintCallable)
	void ShowPlayerName(APawn* InPawn);
	
	UFUNCTION(BlueprintCallable)
    void ShowActorNameOrLabel(APawn* InPawn);

	UFUNCTION(BlueprintCallable)
	void UpdateOverlayHealth(float Health, float MaxHealth);

private:
	
	void ClearPlayerStateTimer(FTimerManager& TimerManager);
	
protected:
	virtual void NativeDestruct() override;


private:
	FTimerHandle PlayerStateTimerHandle;
	FTimerDelegate PlayerStateTimerDelegate;

	UPROPERTY(EditAnywhere)
	float PlayerStateTimerTimeout = 10.0f;

	UPROPERTY(EditAnywhere)
	float PlayerStateTimerInterval = 0.5f;
	
	float TotalTime = 0.0f;
};
