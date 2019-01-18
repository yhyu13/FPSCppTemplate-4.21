// Fill out your copyright notice in the Description page of Project Settings.

#include "PortalC.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"


// Sets default values
APortalC::APortalC()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	/*HangYu-------------------------------------------------------------------------------------*/
	// Initialize components
	// Set size for collision capsule
	RootCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("RootCapsule"));
	RootCapsule->InitCapsuleSize(55.f, 96.0f); // Same size as FPS player
	RootCapsule->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);;
	RootCapsule->RegisterComponent();

	CoordCube = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MyCoordCube"));
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Geometry/Meshes/1M_Cube"));
	if (Mesh)
	{
		CoordCube->SetStaticMesh(Mesh);
	}
	CoordCube->SetHiddenInGame(true);
	CoordCube->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);;
	CoordCube->AttachToComponent(RootCapsule, FAttachmentTransformRules::KeepWorldTransform);
	CoordCube->RegisterComponent();

	SceneCaptureCPP = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MySceneCapture"));
	SceneCaptureCPP->AttachToComponent(RootCapsule, FAttachmentTransformRules::KeepWorldTransform);
	SceneCaptureCPP->RegisterComponent();

	// Intialize variables
	X = FVector(1,0,0);
	Y = FVector(0,1,1);
	Z = FVector(0,0,1);
	Origin = FVector(0.);
	JCoordMat = CreateCoordMatrix(X,Y,Z);
	KCoordMat = CreateCoordMatrix(X, Y, Z);
	JCoordInvMat = CreateCoordMatrix(X, Y, Z);
	KCoordInvMat = CreateCoordMatrix(X, Y, Z);

	PlayerCam = nullptr;
	PortalToCPP = nullptr;
	PlayerRefCPP = nullptr;

	DrawLocalCoord = false;
	ActorTeleportPositiveOffset = 0.001;
}

// Called when the game starts or when spawned
void APortalC::BeginPlay()
{
	Super::BeginPlay();
	UpdateXYZFromCoordCube();
	// These two references must be set in Blueprint

	if (PlayerRefCPP)
		PlayerCam = PlayerRefCPP->GetFirstPersonCameraComponent();
}

//Hang Yu
void APortalC::UpdateXYZFromCoordCube()
{
	if (CoordCube == nullptr)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, "CoordCube is NULL!");
		}
		return;
	}
	// Part 1.
	FVector _X = CoordCube->GetForwardVector();
	FVector _Y = CoordCube->GetRightVector();
	FVector _Z = CoordCube->GetUpVector();
	FVector _Origin = CoordCube->GetComponentLocation();

	// Draw debug Coord Axis
	if (DrawLocalCoord)
	{
		DrawDebugLine(GetWorld(), _Origin, _Origin + _X * 1000, FColor(255, 0, 0), false, -1, 0, 12.333);
		DrawDebugLine(GetWorld(), _Origin, _Origin + _Y * 1000, FColor(0, 255, 0), false, -1, 0, 12.333);
		DrawDebugLine(GetWorld(), _Origin, _Origin + _Z * 1000, FColor(0, 0, 255), false, -1, 0, 12.333);
	}


	if (!X.Equals(_X) || !Y.Equals(_Y) || !Z.Equals(_Y) || !Origin.Equals(_Origin))
	{
		// Part 1.
		X = SetVector(_X);
		Y = SetVector(_Y);
		Z = SetVector(_Z);
		Origin = SetVector(_Origin);
		// Part 2.
		JCoordMat = CreateCoordMatrix(X, Y, Z);
		JCoordInvMat = JCoordMat.Inverse();
		KCoordMat = CreateCoordMatrix(-X, -Y, Z);
		KCoordInvMat = KCoordMat.Inverse();
/*
		DrawDebugMatrix(JCoordMat, FColor::Red);
		DrawDebugMatrix(JCoordInvMat, FColor::Green);
		DrawDebugMatrix(KCoordMat, FColor::Blue);
		DrawDebugMatrix(KCoordInvMat, FColor::Yellow);*/
	}
}

//Hang Yu
FMatrix APortalC::CreateCoordMatrix(const FVector X, const FVector Y, const FVector Z)
{
	// X, Y, Z are columns, W is the scaling vector
	return FMatrix(X, Y, Z, FVector(1, 1, 1));
}

void APortalC::DrawDebugMatrix(const FMatrix Mat, const FColor C)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, C, FString::Printf(TEXT("%s, %s, %s, %s"), \
			*(Mat.GetColumn(0).ToString()), \
			*(Mat.GetColumn(1).ToString()), \
			*(Mat.GetColumn(2).ToString()), \
			*(Mat.GetColumn(3).ToString())));
	}
}

FVector APortalC::SetVector(const FVector V)
{
	return FVector(V.X, V.Y, V.Z);
}

void APortalC::UpdateSceneCaptureWRTPlayerCamera()
{
	if (PortalToCPP == nullptr || PlayerRefCPP == nullptr)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, "PortalToCPP or PlayerRefCPP is NULL!");
		}
		return;
	}

	if (PlayerCam == nullptr)
		PlayerCam = PlayerRefCPP->GetFirstPersonCameraComponent();

	if (SceneCaptureCPP->bEnableClipPlane)
	{
		SceneCaptureCPP->ClipPlaneBase = PortalToCPP->Origin;
		SceneCaptureCPP->ClipPlaneNormal = PortalToCPP->X;
	} 
	// Get 'indentical coordniate matrix' (in terms of portal)
	FMatrix PFromKCoordInvMat = KCoordInvMat;
	FMatrix PToJCoordMat = PortalToCPP->JCoordMat;

	FVector ActorDeltaLocation = PlayerCam->GetComponentLocation() - Origin;
	FVector ActorRotationVector = PlayerCam->GetComponentRotation().Vector();

	// Part 1. Location
	// Teleport Location is X-axis mirroring
	// Use set actor location to teleport
	FVector NewDeltaLocation = FVector((PToJCoordMat.TransformVector(PFromKCoordInvMat.TransformVector(ActorDeltaLocation))));
	FVector NewLocation = PortalToCPP->Origin + NewDeltaLocation;
	// Part 2. Rotation
	// Use player controller to set rotation (using controller Roll, Pitch, Yaw set to 1 s.t. camera rotation = controller rotation)
	FVector NewRotationVector = FVector(PToJCoordMat.TransformVector(PFromKCoordInvMat.TransformVector(ActorRotationVector)));

	// Not only set the location and rotation, but also set the FOV angle (which is important to make clip plane effective visually, otherwise player can see the scene which are clipped)
	SceneCaptureCPP->SetWorldLocationAndRotation(NewLocation, NewRotationVector.Rotation());
	SceneCaptureCPP->FOVAngle = PlayerCam->FieldOfView;
	// Finally capture scene manually (need CaptureEveryFrame set to false)
	SceneCaptureCPP->CaptureScene();
}

// Called every frame
void APortalC::Tick(float DeltaTime)
{
	UpdateXYZFromCoordCube();
	UpdateSceneCaptureWRTPlayerCamera();
	Super::Tick(DeltaTime);
}

