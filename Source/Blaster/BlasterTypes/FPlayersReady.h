#pragma once

#include "FPlayersReady.generated.h"

USTRUCT()
struct FPlayersReady
{
	GENERATED_BODY()

	UPROPERTY()
	FString PlayerName {""};

	UPROPERTY()
	bool bIsReady{false};
};
