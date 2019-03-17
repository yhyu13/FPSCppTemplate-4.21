// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PortalC.h"

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "FPSCppTemplateCharacter.generated.h"

class UInputComponent;

UCLASS(config=Game)
class AFPSCppTemplateCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	class USkeletalMeshComponent* Mesh1P;

	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USkeletalMeshComponent* FP_Gun;

	/** Location on gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USceneComponent* FP_MuzzleLocation;

	/** Gun mesh: VR view (attached to the VR controller directly, no arm, just the actual gun) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USkeletalMeshComponent* VR_Gun;

	/** Location on VR gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USceneComponent* VR_MuzzleLocation;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FirstPersonCameraComponent;

	/** Motion controller (right hand) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UMotionControllerComponent* R_MotionController;

	/** Motion controller (left hand) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UMotionControllerComponent* L_MotionController;

public:
	AFPSCppTemplateCharacter();

	// Begin AActor overrides
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	// End AActor overrides

public:

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Camera)
	float BaseLookUpRate;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector GunOffset;

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class AFPSCppTemplateProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	class USoundBase* FireSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UAnimMontage* FireAnimation;

	/** Whether to use motion controller location for aiming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	uint32 bUsingMotionControllers : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KZ Jump")
	float fMovementMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KZ Jump")
	float fInch2centimeterFactor = 2.54f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KZ Jump")
	float fMinMovement = 635;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KZ Jump")
	float fMovementResetThreshold = .33;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KZ Jump")
	float fBaseMovement = fMinMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KZ Jump")
	float fBaseMovementIncrementRate = 150*2.54;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KZ Jump")
	float fMaxMovement = 3000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KZ Jump")
	float fSyncRate = 0.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KZ Jump")
	bool EnableAutoMoveForward = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Debug")
	bool bPrintTeleport = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Debug")
	bool bPrintSpeed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Debug")
	bool bPrintTurnRate = false;

protected:

	UFUNCTION(BlueprintCallable, Category = "BPI Teleport CPP")
	void TeleportActor(const APortalC* TeleportTo, const APortalC* TeleportFrom);

	AController* PlayerController;
	UCharacterMovementComponent* MovementComponent;

	/** Custom Jump Function with Axis Binding*/
	UFUNCTION(BlueprintCallable, Category = "KZ Jump")
	void JumpByAxis(float value);

	bool bAutoMoveForward = true;

	float FMovementInput = 0.;
	float RMovementInput = 0.;

	/** Reset Sync Rate to 0. */
	UFUNCTION(BlueprintCallable, Category = "KZ Jump")
	void ResetSyncRate();
	
	float fSynRateNumerator = 0.;
	float fSynRateDenominator = 0.;

	/** Fires a projectile. */
	void OnFire();
	
	/** Resets HMD orientation and position in VR. */
	void OnResetVR();

	/** Handles moving forward/backward */
	UFUNCTION(BlueprintCallable, Category = "KZ Jump")
	void KZMoveForward(float Val);

	/** Handles stafing movement, left and right */
	UFUNCTION(BlueprintCallable, Category = "KZ Jump")
	void KZMoveRight(float Val);

	UFUNCTION(BlueprintCallable, Category = "KZ Jump")
	void KZJumpTurn(float Rate);

	UFUNCTION(BlueprintCallable, Category = "KZ Jump")
	void KZJumpLookUp(float Rate);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	struct TouchData
	{
		TouchData() { bIsPressed = false;Location=FVector::ZeroVector;}
		bool bIsPressed;
		ETouchIndex::Type FingerIndex;
		FVector Location;
		bool bMoved;
	};
	void BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location);
	void EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location);
	void TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location);
	TouchData	TouchItem;
	
protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

	/* 
	 * Configures input for touchscreen devices if there is a valid touch interface for doing so 
	 *
	 * @param	InputComponent	The input component pointer to bind controls to
	 * @returns true if touch controls were enabled.
	 */
	bool EnableTouchscreenMovement(UInputComponent* InputComponent);

public:
	/** Returns Mesh1P subobject **/
	FORCEINLINE class USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};

