// Fill out your copyright notice in the Description page of Project Settings.


#include "MCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

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

	//const FString CombinedString = FString::Printf(TEXT("Actor yaw=%f, controller yaw=%f"), GetActorRotation().Yaw, GetControlRotation().Yaw);
	//UE_LOG(LogTemp, Log, TEXT("%s"), *CombinedString);
	//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, CombinedString);

	FRotator ActorRotation = GetActorRotation();
	FRotator ControlRotation = GetControlRotation();
	ControlRotation.Roll = ActorRotation.Roll; // Ignore roll

	// Scale based on how much rotating we're doing
	float RotationDistance = FVector::Distance(ControlRotation.Vector(), ActorRotation.Vector());
	float ScaledLookTowardsSpeedDegrees = LookTowardsSpeedDegrees * LookTowardsSpeedScaling * RotationDistance;

	FVector EndRotation = FMath::VInterpNormalRotationTo(ActorRotation.Vector(), ControlRotation.Vector(), DeltaTime, ScaledLookTowardsSpeedDegrees);
	SetActorRotation(EndRotation.Rotation());
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
	PlayerInputComponent->BindAction("Lunge", IE_Pressed, this, &AMCharacter::Lunge);
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
	// Old: move in the direction of the actor
	// AddMovementInput(GetActorForwardVector(), Value);

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

void AMCharacter::Lunge()
{
	FVector Forwards = GetActorRotation().Vector();
	ensure(Forwards.Normalize());
	GetCharacterMovement()->AddImpulse(Forwards * LungeStrength, false);
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
