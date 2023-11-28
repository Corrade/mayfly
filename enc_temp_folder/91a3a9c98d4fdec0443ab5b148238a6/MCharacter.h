// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class USplineComponent;

UCLASS()
class MAYFLY_API AMCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMCharacter();

protected:
	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* CameraComp;

	UPROPERTY(VisibleAnywhere)
	USplineComponent* TakeoffSplineComp;

	UPROPERTY(EditDefaultsOnly, Category = "Custom parameters")
	float LookTowardsSpeedDegrees = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Custom parameters")
	float LookTowardsSpeedScaling = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Custom parameters")
	float LungeStrength = 250000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Custom parameters")
	float BackstepStrength = 150000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Custom parameters")
	float SpringArmVelocityFactor = 0.05f;

	UPROPERTY(EditDefaultsOnly, Category = "Custom parameters")
	float MaxVelocity = 500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Custom parameters")
	float TakeoffDurationSec = 1.0f;

	FTimerHandle TimerHandle_Takeoff;
	FVector LocationBeforeTakeoff;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void Landed(const FHitResult& Hit) override;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Takeoff();
	void TakeoffEnded();
	void Backstep();
	void StartFlying();
	void VisualiseRotations();
};

	/**
	* Actor's current movement mode (walking, falling, etc).
	*    - walking:  Walking on a surface, under the effects of friction, and able to "step up" barriers. Vertical velocity is zero.
	*    - falling:  Falling under the effects of gravity, after jumping or walking off the edge of a surface.
	*    - flying:   Flying, ignoring the effects of gravity.
	*    - swimming: Swimming through a fluid volume, under the effects of gravity and buoyancy.
	*    - custom:   User-defined custom movement mode, including many possible sub-modes.
	* This is automatically replicated through the Character owner and for client-server movement functions.
	* @see SetMovementMode(), CustomMovementMode
	*/
//UPROPERTY(Category = "Character Movement: MovementMode", BlueprintReadOnly)
//TEnumAsByte<enum EMovementMode> MovementMode; 
