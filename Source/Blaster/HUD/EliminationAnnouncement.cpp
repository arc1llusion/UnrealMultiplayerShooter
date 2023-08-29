// Fill out your copyright notice in the Description page of Project Settings.


#include "EliminationAnnouncement.h"

#include "Components/TextBlock.h"

void UEliminationAnnouncement::SetEliminationAnnouncementText(const FString& AttackerName, const FString& VictimName) const
{
	if(AnnouncementText)
	{
		const FString EliminationAnnouncementText = FString::Printf(TEXT("%s eliminated %s"), *AttackerName, *VictimName);
		AnnouncementText->SetText(FText::FromString(EliminationAnnouncementText));
	}
}
