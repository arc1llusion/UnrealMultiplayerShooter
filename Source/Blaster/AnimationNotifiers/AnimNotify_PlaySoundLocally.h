// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify_PlaySound.h"
#include "AnimNotify_PlaySoundLocally.generated.h"

/**
 * 
 */
UCLASS(Const, HideCategories=Object, CollapseCategories, Config = Game, meta=(DisplayName="Play Sound Locally"))
class BLASTER_API UMyAnimNotify_PlaySound : public UAnimNotify_PlaySound
{
	GENERATED_BODY()

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
