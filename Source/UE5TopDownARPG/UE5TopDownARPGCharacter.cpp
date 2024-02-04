// Copyright Epic Games, Inc. All Rights Reserved.

#include "UE5TopDownARPGCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "Abilities/BaseAbility.h"
#include "UE5TopDownARPGGameMode.h"
#include "UE5TopDownARPG.h"
#include "GameFramework/PhysicsVolume.h"
#include "DrawDebugHelpers.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"

AUE5TopDownARPGCharacter::AUE5TopDownARPGCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	ClimbingBoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("ClimbingBoxComponent"));
	ClimbingBoxComponent->SetupAttachment(RootComponent);
	ClimbingBoxComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Overlap);
	ClimbingBoxComponent->SetGenerateOverlapEvents(true);

	ClimbJumpArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ClimbJumpArrowComponent"));
	ClimbJumpArrowComponent->SetupAttachment(RootComponent);
	ClimbJumpArrowComponent->ArrowLength = ClimbJumpArrowMaxLength;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = OriginalCameraDistance;
	CameraBoom->SetRelativeRotation(FRotator(OriginalCameraPitch, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AUE5TopDownARPGCharacter::BeginPlay()
{
	Super::BeginPlay();

	IKOffsetRightHand = FVector::ZeroVector;
	IKOffsetLeftHand = FVector::ZeroVector;

	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (IsValid(CharacterMesh) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("AUE5TopDownARPGCharacter::BeginPlay IsValid(Mesh) == false"));
		return;
	}
	FVector GrabSocketLocation = CharacterMesh->GetSocketLocation(GrabSocketName);
	FVector GrabRelativeLocation = GrabSocketLocation - GetActorLocation();
	ClimbJumpArrowComponent->SetRelativeLocation(GrabRelativeLocation);
	ClimbJumpArrowComponent->SetVisibility(false);

	OnTakeAnyDamage.AddDynamic(this, &AUE5TopDownARPGCharacter::TakeAnyDamage);

	FScriptDelegate OnClimbingComponentBeginOverlapDelegate;
	OnClimbingComponentBeginOverlapDelegate.BindUFunction(this, "OnClimbingComponentBeginOverlap");
	ClimbingBoxComponent->OnComponentBeginOverlap.AddUnique(OnClimbingComponentBeginOverlapDelegate);
	
	if (AbilityTemplate != nullptr)
	{
		AbilityInstance = NewObject<UBaseAbility>(this, AbilityTemplate);
	}
}

void AUE5TopDownARPGCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

	UE_LOG(LogUE5TopDownARPG, Warning, TEXT("Right Hand: %s"), *IKOffsetRightHand.ToString());
	UE_LOG(LogUE5TopDownARPG, Warning, TEXT("Left Hand: %s"), *IKOffsetLeftHand.ToString());

	/** Use this to get the controller for the given IKRig */
	//static UIKRigController* GetIKRigController(UIKRigDefinition * InIKRigDefinition);

	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (IsValid(CharacterMesh) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("AUE5TopDownARPGCharacter::Tick IsValid(Mesh) == false"));
		return;
	}

	FVector GrabSocketLocation = CharacterMesh->GetSocketLocation(GrabSocketName);
	FRotator CameraBoomRotation = CameraBoom->GetComponentRotation();
	float CameraDistance = CameraBoom->TargetArmLength;
	if (IsValid(GrabbedHold))
	{
		const FVector& HoldLocation = GrabbedHold->GetActorLocation();
	
		FVector RightHandLocation = CharacterMesh->GetSocketLocation(RightHandSocketName);
		FVector LeftHandLocation = CharacterMesh->GetSocketLocation(LeftHandSocketName);

		IKOffsetRightHand = HoldLocation - RightHandLocation;
		IKOffsetLeftHand = HoldLocation - LeftHandLocation;
		//IKOffsetRightHand = FMath::VInterpTo(IKOffsetRightHand, HoldLocation - RightHandLocation, DeltaSeconds, PullToHoldForce);
		//IKOffsetLeftHand = FMath::VInterpTo(IKOffsetLeftHand, HoldLocation - LeftHandLocation, DeltaSeconds, PullToHoldForce);

		float HoldDistance = FVector::Distance(GrabSocketLocation, HoldLocation);
		if (HoldDistance > GrabDistanceTreshold)
		{
			GetCharacterMovement()->StopMovementImmediately();
			FVector NewLocation = FMath::VInterpTo(GrabSocketLocation, HoldLocation, DeltaSeconds, PullToHoldForce);
			FVector LocationOffset = NewLocation - GrabSocketLocation;
			SetActorLocation(GetActorLocation() + LocationOffset);
		}

		if (GetActorRotation().Equals(DesiredRotation) == false)
		{
			FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaSeconds, PullToHoldForce);
			SetActorRotation(NewRotation);
		}

		if (FMath::IsNearlyEqual(CameraBoomRotation.Pitch, ClimbCameraPitch) == false)
		{
			CameraBoomRotation.Pitch = FMath::FInterpTo(CameraBoomRotation.Pitch, ClimbCameraPitch, DeltaSeconds, PullToHoldForce);
			CameraBoom->SetWorldRotation(CameraBoomRotation);
		}
		if (FMath::IsNearlyEqual(CameraDistance, ClimbCameraDistance) == false)
		{
			CameraBoom->TargetArmLength = FMath::FInterpTo(CameraDistance, ClimbCameraDistance, DeltaSeconds, PullToHoldForce);
		}
	}
	else
	{
		if (FMath::IsNearlyEqual(CameraBoomRotation.Pitch, OriginalCameraPitch) == false)
		{
			CameraBoomRotation.Pitch = FMath::FInterpTo(CameraBoomRotation.Pitch, OriginalCameraPitch, DeltaSeconds, PullToHoldForce);
			CameraBoom->SetWorldRotation(CameraBoomRotation);
		}
		if (FMath::IsNearlyEqual(CameraDistance, OriginalCameraDistance) == false)
		{
			CameraBoom->TargetArmLength = FMath::FInterpTo(CameraDistance, OriginalCameraDistance, DeltaSeconds, PullToHoldForce);
		}
	}
}

void AUE5TopDownARPGCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AUE5TopDownARPGCharacter, Health);
}

void AUE5TopDownARPGCharacter::SetIsJumpArrowVisible(bool IsVisible)
{
	if (IsValid(ClimbJumpArrowComponent) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("AUE5TopDownARPGCharacter::SetIsJumpArrowVisible IsValid(ClimbJumpArrowComponent) == false"));
		return;
	}

	ClimbJumpArrowComponent->SetVisibility(IsVisible);
}

void AUE5TopDownARPGCharacter::UpdateJumpArrow(const FVector2D& Direction, float MaxLengthFraction)
{
	if (IsValid(ClimbJumpArrowComponent) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("AUE5TopDownARPGCharacter::UpdateJumpArrowProperties IsValid(ClimbJumpArrowComponent) == false"));
		return;
	}

	ClimbJumpArrowComponent->SetWorldScale3D(FVector(MaxLengthFraction,1.f,1.f));
	ClimbJumpArrowComponent->SetRelativeRotation(FVector(0.f, Direction.X, Direction.Y).ToOrientationRotator());
}

bool AUE5TopDownARPGCharacter::ActivateAbility(FVector Location)
{
	if (IsValid(AbilityInstance))
	{
		return AbilityInstance->Activate(Location);
	}
	return false;
}

void AUE5TopDownARPGCharacter::TakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigateBy, AActor* DamageCauser)
{
	Health -= Damage;
	OnRep_SetHealth(Health + Damage);
	UE_LOG(LogUE5TopDownARPG, Log, TEXT("Health %f"), Health);
	if (Health <= 0.0f)
	{
		FTimerManager& TimerManager = GetWorld()->GetTimerManager();
		if (TimerManager.IsTimerActive(DeathHandle) == false)
		{
			GetWorld()->GetTimerManager().SetTimer(DeathHandle, this, &AUE5TopDownARPGCharacter::Death, DeathDelay);
		}
	}
}

void AUE5TopDownARPGCharacter::OnClimbingComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (IsValid(OtherActor) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("AUE5TopDownARPGCharacter::OnClimbingComponentBeginOverlap IsValid(OtherActor) == false"));
		return;
	}

	if (OtherActor->ActorHasTag(ClimbingHoldsActorTag) && GetCharacterMovement()->IsFalling())
	{
		UE_LOG(LogUE5TopDownARPG, Warning, TEXT("Grabbed"));
		GrabHold(OtherActor, OtherActor->GetActorLocation());
	}
}

void AUE5TopDownARPGCharacter::OnRep_SetHealth(float OldHealth)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Health %f"), Health));
	}
}

void AUE5TopDownARPGCharacter::GrabHold(AActor* Hold, const FVector& OverlapLocation)
{
	if (IsValid(Hold) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("AUE5TopDownARPGCharacter::GrabHold IsValid(Hold) == false"));
		return;
	}

	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
	GrabbedHold = Hold;

	DesiredRotation = GetFaceWallRotation();

	OnHoldGrabbedDelegate.ExecuteIfBound(Hold);
}

void AUE5TopDownARPGCharacter::ReleaseHold()
{
	if (IsValid(GrabbedHold))
	{
		OnHoldReleasedDelegate.ExecuteIfBound(GrabbedHold);
		GrabbedHold = nullptr;
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
	}

	IKOffsetRightHand = FVector::ZeroVector;
	IKOffsetLeftHand = FVector::ZeroVector;
}

FRotator AUE5TopDownARPGCharacter::GetFaceWallRotation() const
{
	UWorld* World = GetWorld();
	if(IsValid(World) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("AUE5TopDownARPGCharacter::GetNormalVectorFromWall IsValid(World) == false"));
		return FRotator::ZeroRotator;
	}
	const FVector Start = GetActorLocation();
	const FVector End = Start + GetActorForwardVector() * ClimbingBoxComponent->GetScaledBoxExtent().Y * 1.5;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	FHitResult Hit;
	World->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_WorldStatic, QueryParams);

	const FVector Direction = -Hit.ImpactNormal;
	return Direction.ToOrientationRotator();
}

void AUE5TopDownARPGCharacter::ClimbJump(FVector2D InDirection, float MaxForceFraction)
{
	if (IsValid(GrabbedHold))
	{
		ReleaseHold();
		FVector JumpDirection = FVector{ 0.f, InDirection.X, InDirection.Y };
		GetCharacterMovement()->AddImpulse(JumpDirection * GetCharacterMovement()->Mass * ClimbJumpMaxForce * MaxForceFraction);
	}
}

void AUE5TopDownARPGCharacter::Death()
{
	UE_LOG(LogUE5TopDownARPG, Log, TEXT("Death"));
	AUE5TopDownARPGGameMode* GameMode = Cast<AUE5TopDownARPGGameMode>(GetWorld()->GetAuthGameMode());
	if (IsValid(GameMode))
	{
		GameMode->EndGame(false);
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FVector Location = GetActorLocation();
	FRotator Rotation = GetActorRotation();
	if (FMath::RandBool())
	{
		AActor* SpawnedActor = GetWorld()->SpawnActor(AfterDeathSpawnClass, &Location, &Rotation, SpawnParameters);
	}

	GetWorld()->GetTimerManager().ClearTimer(DeathHandle);
	Destroy();
}
