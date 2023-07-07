// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify_PlaySoundLocally.h"

#include "Blaster/Character/BlasterCharacter.h"

void UMyAnimNotify_PlaySound::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                     const FAnimNotifyEventReference& EventReference)
{
	if(!MeshComp)
	{
		return;
	}

#if WITH_EDITORONLY_DATA
	Super::Notify(MeshComp, Animation, EventReference);
#else	
	if(const ABlasterCharacter* Owner = Cast<ABlasterCharacter>(MeshComp->GetOwner()))
	{
		if(Owner->IsLocallyControlled())
		{
			Super::Notify(MeshComp, Animation, EventReference);
		}
	}
#endif
}
