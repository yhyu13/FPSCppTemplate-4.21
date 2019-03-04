// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "FPSCppTemplateCharacter.h"
#include "FPSCppTemplateProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId
#include "Engine.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AFPSCppTemplateCharacter

AFPSCppTemplateCharacter::AFPSCppTemplateCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->RelativeLocation = FVector(-39.56f, 1.75f, 64.f); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->RelativeRotation = FRotator(1.9f, -19.19f, 5.2f);
	Mesh1P->RelativeLocation = FVector(-0.5f, -4.4f, -155.7f);

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.

	// Create VR Controllers.
	R_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("R_MotionController"));
	R_MotionController->MotionSource = FXRMotionControllerBase::RightHandSourceId;
	R_MotionController->SetupAttachment(RootComponent);
	L_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("L_MotionController"));
	L_MotionController->SetupAttachment(RootComponent);

	// Create a gun and attach it to the right-hand VR controller.
	// Create a gun mesh component
	VR_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VR_Gun"));
	VR_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	VR_Gun->bCastDynamicShadow = false;
	VR_Gun->CastShadow = false;
	VR_Gun->SetupAttachment(R_MotionController);
	VR_Gun->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	VR_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("VR_MuzzleLocation"));
	VR_MuzzleLocation->SetupAttachment(VR_Gun);
	VR_MuzzleLocation->SetRelativeLocation(FVector(0.000004, 53.999992, 10.000000));
	VR_MuzzleLocation->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));		// Counteract the rotation of the VR gun model.

	PlayerController = nullptr;
	MovementComponent = nullptr;
}

void AFPSCppTemplateCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	if (PlayerController == nullptr) PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (MovementComponent == nullptr) MovementComponent = GetCharacterMovement();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));

	// Show or hide the two versions of the gun based on whether or not we're using motion controllers.
	if (bUsingMotionControllers)
	{
		VR_Gun->SetHiddenInGame(false, true);
		Mesh1P->SetHiddenInGame(true, true);
	}
	else
	{
		VR_Gun->SetHiddenInGame(true, true);
		Mesh1P->SetHiddenInGame(false, true);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AFPSCppTemplateCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAxis("Jump", this, &AFPSCppTemplateCharacter::JumpByAxis);

	// Bind fire event
	PlayerInputComponent->BindAction("Grenade", IE_Pressed, this, &AFPSCppTemplateCharacter::OnFire);

	// Enable touchscreen input
	EnableTouchscreenMovement(PlayerInputComponent);

	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AFPSCppTemplateCharacter::OnResetVR);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AFPSCppTemplateCharacter::KZMoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AFPSCppTemplateCharacter::KZMoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &AFPSCppTemplateCharacter::KZJumpTurn);
	PlayerInputComponent->BindAxis("TurnRate", this, &AFPSCppTemplateCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &AFPSCppTemplateCharacter::KZJumpLookUp);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AFPSCppTemplateCharacter::LookUpAtRate);
}

void AFPSCppTemplateCharacter::TeleportActor(const APortalC* TeleportTo, const APortalC* TeleportFrom)
{
	// Get 'indentical coordniate matrix' (in terms of portal)
	FMatrix PFromKCoordInvMat = TeleportFrom->KCoordInvMat;
	FMatrix PToJCoordMat = TeleportTo->JCoordMat;

	FVector ActorDeltaLocation = GetActorLocation() - TeleportFrom->Origin;
	FVector ActorRotationVector = GetFirstPersonCameraComponent()->GetComponentRotation().Vector();
	FVector Velocity = GetVelocity();

	if (PlayerController == nullptr) PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (MovementComponent == nullptr) MovementComponent = GetCharacterMovement();

	// Part 1. Location
	// Teleport Location is X-axis mirroring
	// Use set actor location to teleport
	FVector temp1 = PFromKCoordInvMat.TransformVector(ActorDeltaLocation);
	// Must set the X component of the portal-frame location vecotr to a positive number
	// In order to prevent triggering the teleport condition in the other portal
	temp1.X = TeleportTo->ActorTeleportPositiveOffset;
	FVector NewDeltaLocation = FVector((PToJCoordMat.TransformVector(temp1)));
	FVector NewLocation = TeleportTo->Origin + NewDeltaLocation;
	SetActorLocation(NewLocation,false,nullptr,ETeleportType::None);

	// Part 2. Rotation
	// Use player controller to set rotation (using controller Roll, Pitch, Yaw set to 1 s.t. camera rotation = controller rotation)
	FVector NewRotationVector = FVector(PToJCoordMat.TransformVector(PFromKCoordInvMat.TransformVector(ActorRotationVector)));
	PlayerController->SetControlRotation(NewRotationVector.Rotation()); // #include "Kismet/GameplayStatics.h"

	// Part 3. Velocity
	// Use chracter movement component to set velocity
	FVector NewVelocity = FVector(PToJCoordMat.TransformVector(PFromKCoordInvMat.TransformVector(Velocity)));
	MovementComponent->Velocity = NewVelocity; // #include "GameFramework/CharacterMovementComponent.h"

	if (GEngine && bPrintTeleport)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Blue, FString::Printf(TEXT("Old V %f, New V %f"), Velocity.Size(), NewVelocity.Size()));
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, NewVelocity.ToString());
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, NewDeltaLocation.ToString());
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, NewRotationVector.ToString());
	}
}

void AFPSCppTemplateCharacter::OnFire()
{
	// try and fire a projectile
	if (ProjectileClass != NULL)
	{
		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			if (bUsingMotionControllers)
			{
				const FRotator SpawnRotation = VR_MuzzleLocation->GetComponentRotation();
				const FVector SpawnLocation = VR_MuzzleLocation->GetComponentLocation();
				World->SpawnActor<AFPSCppTemplateProjectile>(ProjectileClass, SpawnLocation, SpawnRotation);
			}
			else
			{
				const FRotator SpawnRotation = GetControlRotation();
				// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
				const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);

				//Set Spawn Collision Handling Override
				FActorSpawnParameters ActorSpawnParams;
				ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

				// spawn the projectile at the muzzle
				World->SpawnActor<AFPSCppTemplateProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
			}
		}
	}

	// try and play the sound if specified
	//if (FireSound != NULL)
	//{
	//	UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	//}

	// try and play a firing animation if specified
	if (FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void AFPSCppTemplateCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AFPSCppTemplateCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == true)
	{
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false))
	{
		OnFire();
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AFPSCppTemplateCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	TouchItem.bIsPressed = false;
}

void AFPSCppTemplateCharacter::JumpByAxis(float Value)
{
	if (Value != 0.0f)
	{
		this->Jump();
	}
	else
	{
		this->StopJumping();
	}
}

void AFPSCppTemplateCharacter::KZMoveForward(float Value)
{
	if (MovementComponent == nullptr) MovementComponent = GetCharacterMovement();
	FMovementInput = Value;
	FVector downVector = FVector(0, 0, -1);
	FVector forwardVector = GetActorForwardVector() - FVector::DotProduct(GetActorForwardVector(), downVector) * downVector;

	if (Value != 0.0f)
	{
		FMovementInput /= FMath::Sqrt((pow(RMovementInput, 2) + pow(FMovementInput, 2)));
		AddMovementInput(forwardVector.GetSafeNormal(), FMovementInput);

		// Disable auto move forward input (e.g. Pressing "W" key) if a negative input is received (e.g. Pressing "S" key)
		if (Value < 0.0f && MovementComponent->IsFalling())
		{
			bAutoMoveForward = false;
		}
	}

	// Enable auto move forward input (e.g. Like pressing "W" key in the air)
	if (EnableAutoMoveForward)
	{
		if (MovementComponent->IsFalling() && bAutoMoveForward)
		{
			FMovementInput = 1.0f;
			FMovementInput /= FMath::Sqrt((pow(RMovementInput, 2) + pow(FMovementInput, 2)));
			AddMovementInput(forwardVector.GetSafeNormal(), FMovementInput);
		}
	}
}

void AFPSCppTemplateCharacter::KZMoveRight(float Value)
{
	RMovementInput = Value;

	if (Value != 0.0f)
	{
		RMovementInput /= FMath::Sqrt((pow(RMovementInput, 2) + pow(FMovementInput, 2)));
		AddMovementInput(GetActorRightVector(), RMovementInput);
	}
}

void AFPSCppTemplateCharacter::KZJumpTurn(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate* BaseTurnRate * GetWorld()->GetDeltaSeconds());
	if (MovementComponent == nullptr) MovementComponent = GetCharacterMovement();

	if (MovementComponent->IsFalling())
	{
		float DeltaSpeed = fBaseMovementIncrementRate * fMovementMultiplier * GetWorld()->GetDeltaSeconds();

		if (MovementComponent->MaxWalkSpeed < fMaxMovement && MovementComponent->MaxWalkSpeedCrouched < fMaxMovement)
		{
			if (Rate * RMovementInput > 0.f)
			{
				if (fabs(Rate) >= .99f && fabs(Rate) < 10.0f)
				{
					fSynRateNumerator += 1.;

					MovementComponent->MaxWalkSpeed += DeltaSpeed;
					MovementComponent->MaxWalkSpeedCrouched += DeltaSpeed;
				}
				fSynRateDenominator += 1.;
			}
		}
	}

	if (GEngine && bPrintTurnRate)
	{
		GEngine->AddOnScreenDebugMessage(-1, -1.f, FColor::FColor(0, 255, 255), FString::Printf(TEXT("Camera Turn Rate (Horizontal): %.2f "), Rate));
	}
}

void AFPSCppTemplateCharacter::KZJumpLookUp(float Rate)
{
	AddControllerPitchInput(Rate* BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AFPSCppTemplateCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AFPSCppTemplateCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

bool AFPSCppTemplateCharacter::EnableTouchscreenMovement(class UInputComponent* PlayerInputComponent)
{
	if (FPlatformMisc::SupportsTouchInput() || GetDefault<UInputSettings>()->bUseMouseForTouch)
	{
		PlayerInputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AFPSCppTemplateCharacter::BeginTouch);
		PlayerInputComponent->BindTouch(EInputEvent::IE_Released, this, &AFPSCppTemplateCharacter::EndTouch);

		//Commenting this out to be more consistent with FPS BP template.
		//PlayerInputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AFPSCppTemplateCharacter::TouchUpdate);
		return true;
	}
	
	return false;
}

void AFPSCppTemplateCharacter::ResetSyncRate()
{
	fSyncRate = 0.;
}

void AFPSCppTemplateCharacter::Tick(float DeltaSeconds)
{
	// Call any parent class Tick implementation
	Super::Tick(DeltaSeconds);

	if (MovementComponent == nullptr) MovementComponent = GetCharacterMovement();

	// Ajust movement by multiplier
	if (MovementComponent->Velocity.Size2D() <= (fMinMovement * fMovementMultiplier * fMovementResetThreshold))
	{
		MovementComponent->MaxWalkSpeed = fMinMovement * fMovementMultiplier;
		MovementComponent->MaxWalkSpeedCrouched = fMinMovement * fMovementMultiplier;
	}

	// Condition to turn on auto Move forward while falling
	if (EnableAutoMoveForward)
	{
		if (!MovementComponent->IsFalling() && !bAutoMoveForward)
		{
			bAutoMoveForward = true;
		}
	}

	// Calculate Sync Rate
	if (fSynRateDenominator == 0.)
	{
		fSyncRate = 0.;
	}
	else
	{
		fSyncRate = fSynRateNumerator / fSynRateDenominator;
	}

	if (GEngine && bPrintSpeed)
	{
		// Display speed in cm/s or inch/s
		GEngine->AddOnScreenDebugMessage(-1, -1.f, FColor::FColor(0,255,255), FString::Printf(TEXT("Speed: %.2f units/s"), MovementComponent->Velocity.Size2D() / fInch2centimeterFactor));
	}
}