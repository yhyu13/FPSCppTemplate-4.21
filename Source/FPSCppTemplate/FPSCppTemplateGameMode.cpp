// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "FPSCppTemplateGameMode.h"
#include "FPSCppTemplateHUD.h"
#include "FPSCppTemplateCharacter.h"
#include "UObject/ConstructorHelpers.h"

AFPSCppTemplateGameMode::AFPSCppTemplateGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/MyFirstPerson/Blueprints/FPSCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AFPSCppTemplateHUD::StaticClass();
}
