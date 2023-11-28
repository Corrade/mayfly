// Fill out your copyright notice in the Description page of Project Settings.


#include "MCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SplineComponent.h"

// Sets default values
AMCharacter::AMCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>("SpringArmComp");
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->bUsePawnControlRotation = true;

	CameraComp = CreateDefaultSubobject<UCameraComponent>("CameraComp");
	CameraComp->SetupAttachment(SpringArmComp);

	TakeoffSplineComp = CreateDefaultSubobject<USplineComponent>("TakeoffSplineComp");
	TakeoffSplineComp->SetupAttachment(RootComponent);

	// Prevent automatic syncing to the controller's rotation
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;

}

// Called when the game starts or when spawned
void AMCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AMCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	GetCharacterMovement()->VisualizeMovement();
	VisualiseRotations();

	// Implement takeoff movement by locking to spline - replace SetActorLocation() with AddMovementInput() to support blending with other movement abilities?
	if (GetWorldTimerManager().IsTimerActive(TimerHandle_Takeoff))
	{
		// Percent of timer elapsed; linear
		float SplineProgressPercent = GetWorldTimerManager().GetTimerElapsed(TimerHandle_Takeoff) / TakeoffDurationSec;
		float SplineProgressDistance = SplineProgressPercent * TakeoffSplineComp->GetSplineLength();
		FVector SplineLocationLocalOffset = TakeoffSplineComp->GetLocationAtDistanceAlongSpline(SplineProgressDistance, ESplineCoordinateSpace::Local);
		FVector SplineLocation = LocationBeforeTakeoff + GetActorRotation().RotateVector(SplineLocationLocalOffset);
		SetActorLocation(SplineLocation);
		return;
	}

	//const FString CombinedString = FString::Printf(TEXT("Actor yaw=%f, controller yaw=%f"), GetActorRotation().Yaw, GetControlRotation().Yaw);
	//UE_LOG(LogTemp, Log, TEXT("%s"), *CombinedString);
	//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, CombinedString);

	// Lerp rotation towards the controller
	FRotator ActorRotation = GetActorRotation();
	FRotator ControlRotation = GetControlRotation();
	ControlRotation.Roll = ActorRotation.Roll; // Ignore roll
	float RotationDistance = FVector::Distance(ControlRotation.Vector(), ActorRotation.Vector()); // Scale based on how much rotating we're doing
	float ScaledLookTowardsSpeedDegrees = LookTowardsSpeedDegrees * LookTowardsSpeedScaling * RotationDistance;
	FVector EndRotation = FMath::VInterpNormalRotationTo(ActorRotation.Vector(), ControlRotation.Vector(), DeltaTime, ScaledLookTowardsSpeedDegrees);

	// If grounded, avoid pointing down
	if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Walking)
	{
		EndRotation.Z = FMath::Max(0, EndRotation.Z);
	}

	// Apply the above rotation changes
	SetActorRotation(EndRotation.Rotation());
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, EndRotation.ToString());

	// Limit velocity
	if (GetCharacterMovement()->Velocity.Length() > MaxVelocity)
	{
		FVector NewVelocity = GetCharacterMovement()->Velocity;
		ensure(NewVelocity.Normalize());
		NewVelocity *= MaxVelocity;
		GetCharacterMovement()->Velocity = NewVelocity;
	}

	// Scale spring arm based on velocity
	SpringArmComp->TargetArmLength = FMath::Lerp(SpringArmComp->TargetArmLength, FMath::Min(400.0f + static_cast<float>(GetCharacterMovement()->Velocity.Length() * SpringArmVelocityFactor), 1000.0f), DeltaTime);
}

// Called to bind functionality to input
void AMCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AMCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Yaw", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Pitch", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMCharacter::Jump);
	PlayerInputComponent->BindAction("Lunge", IE_Pressed, this, &AMCharacter::Takeoff);
	PlayerInputComponent->BindAction("Backstep", IE_Pressed, this, &AMCharacter::Backstep);
	PlayerInputComponent->BindAction("StartFlying", IE_Pressed, this, &AMCharacter::StartFlying);
}

void AMCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	UE_LOG(LogTemp, Log, TEXT("AMCharacter::Landed()"));
}

void AMCharacter::MoveForward(float Value)
{
	// New: move in the direction of the controller
	FRotator ControlRot = GetControlRotation();
	ControlRot.Roll = 0.0f;
	AddMovementInput(ControlRot.Vector(), Value);
}

void AMCharacter::MoveRight(float Value)
{
	FRotator ControlRot = GetControlRotation();
	ControlRot.Pitch = 0.0f;
	ControlRot.Roll = 0.0f;
	// Logic borrowed from Blueprint math library - UKismetMathLibrary::GetRightVector()
	FVector RightVector = FRotationMatrix(ControlRot).GetScaledAxis(EAxis::Y);
	AddMovementInput(RightVector, Value);
}

void AMCharacter::Takeoff()
{
	if (GetWorldTimerManager().IsTimerActive(TimerHandle_Takeoff))
	{
		return;
	}

	//FVector Forwards = GetActorRotation().Vector();
	//ensure(Forwards.Normalize());
	//GetCharacterMovement()->AddImpulse(Forwards * LungeStrength, false);
	LocationBeforeTakeoff = GetActorLocation();
	GetWorldTimerManager().SetTimer(TimerHandle_Takeoff, this, &AMCharacter::TakeoffEnded, TakeoffDurationSec);
}

void AMCharacter::TakeoffEnded()
{
	const FVector SplineEnd = TakeoffSplineComp->GetLocationAtDistanceAlongSpline(0.99 * TakeoffSplineComp->GetSplineLength(), ESplineCoordinateSpace::Local);
	const FVector SplineEndMinusEpsilon = TakeoffSplineComp->GetLocationAtDistanceAlongSpline(0.98 * TakeoffSplineComp->GetSplineLength(), ESplineCoordinateSpace::Local);
	FVector SplineEndingVectorLocal = SplineEnd - SplineEndMinusEpsilon;

	FVector SplineEndingVector = GetActorRotation().RotateVector(SplineEndingVectorLocal);
	SplineEndingVector.Normalize();

	GetCharacterMovement()->AddImpulse(SplineEndingVector * LungeStrength, false);
}

void AMCharacter::Backstep()
{
	FVector Backwards = -1.0f * GetActorRotation().Vector();
	ensure(Backwards.Normalize());
	GetCharacterMovement()->AddImpulse(Backwards * BackstepStrength, false);
}

void AMCharacter::StartFlying()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
}

void AMCharacter::VisualiseRotations()
{
	// -- Rotation Visualization -- //
	const float DrawScale = 100.0f;
	const float Thickness = 5.0f;

	FVector LineStart = GetActorLocation();
	// Offset to the right of pawn
	LineStart += GetActorRightVector() * 100.0f;
	// Set line end in direction of the actor's forward
	FVector ActorDirection_LineEnd = LineStart + (GetActorForwardVector() * 100.0f);
	// Draw Actor's Direction
	DrawDebugDirectionalArrow(GetWorld(), LineStart, ActorDirection_LineEnd, DrawScale, FColor::Yellow, false, 0.0f, 0, Thickness);

	FVector ControllerDirection_LineEnd = LineStart + (GetControlRotation().Vector() * 100.0f);
	// Draw 'Controller' Rotation ('PlayerController' that 'possessed' this character)
	DrawDebugDirectionalArrow(GetWorld(), LineStart, ControllerDirection_LineEnd, DrawScale, FColor::Green, false, 0.0f, 0, Thickness);
}
