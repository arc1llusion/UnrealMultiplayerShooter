// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify_PlaySoundLocally.h"

#include "Blaster/Character/BlasterCharacter.h"

void UMyAnimNotify_PlaySound::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                     const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if(!MeshComp)
	{
		return;
	}

	if(const ABlasterCharacter* Owner = Cast<ABlasterCharacter>(MeshComp->GetOwner()))
	{
		if(Owner->IsLocallyControlled())
		{
			Super::Notify(MeshComp, Animation, EventReference);
		}
	}
}
