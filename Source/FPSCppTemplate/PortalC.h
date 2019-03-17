// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/GameEngine.h"
#include "FPSCppTemplateCharacter.h"
#include "PortalC.generated.h"

UCLASS()
class FPSCPPTEMPLATE_API APortalC : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APortalC();

	// Variables
	FVector X;
	FVector Y;
	FVector Z;
	FVector Origin;

	FMatrix JCoordMat;
	FMatrix KCoordMat;
	FMatrix JCoordInvMat;
	FMatrix KCoordInvMat;

	class UCameraComponent* PlayerCam;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Utilities Hang Yu
	void UpdateXYZFromCoordCube();
	FMatrix CreateCoordMatrix(const FVector X, const FVector Y, const FVector Z);
	void DrawDebugMatrix(const FMatrix Mat, const FColor C);
	FVector SetVector(const FVector V);

	void UpdateSceneCaptureWRTPlayerCamera();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, category = References)
	class UCapsuleComponent* RootCapsule;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, category = References)
	UStaticMeshComponent* CoordCube;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, category = References)
	USceneCaptureComponent2D* SceneCaptureCPP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, category = References)
	class APortalC* PortalToCPP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, category = References)
	class AFPSCppTemplateCharacter* PlayerRefCPP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Debug")
	bool bPrintPlayerRefNull = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, category = DebugCPP)
	bool DrawLocalCoord;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, category = DebugCPP)
	float ActorTeleportPositiveOffset;
};
