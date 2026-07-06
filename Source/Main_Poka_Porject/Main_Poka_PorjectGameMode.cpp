// Copyright Epic Games, Inc. All Rights Reserved.

#include "Main_Poka_PorjectGameMode.h"
#include "Main_Poka_PorjectCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMain_Poka_PorjectGameMode::AMain_Poka_PorjectGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
