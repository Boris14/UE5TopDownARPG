// Copyright Epic Games, Inc. All Rights Reserved.

#include "UE5TopDownARPGCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SphereComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "Abilities/BaseAbility.h"
#include "UE5TopDownARPGGameMode.h"
#include "UE5TopDownARPG.h"
#include "GameFramework/PhysicsVolume.h"
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

	ClimbingSphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("ClimbingSphereComponent"));
	ClimbingSphereComponent->SetupAttachment(RootComponent);
	ClimbingSphereComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Overlap);
	ClimbingSphereComponent->SetGenerateOverlapEvents(true);

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
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
	
	OnTakeAnyDamage.AddDynamic(this, &AUE5TopDownARPGCharacter::TakeAnyDamage);

	FScriptDelegate OnClimbingComponentBeginOverlapDelegate;
	OnClimbingComponentBeginOverlapDelegate.BindUFunction(this, "OnClimbingComponentBeginOverlap");
	ClimbingSphereComponent->OnComponentBeginOverlap.AddUnique(OnClimbingComponentBeginOverlapDelegate);

	if (AbilityTemplate != nullptr)
	{
		AbilityInstance = NewObject<UBaseAbility>(this, AbilityTemplate);
	}
}

void AUE5TopDownARPGCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

	if (IsValid(GrabbedHold))
	{
		GetController()->SetIgnoreMoveInput(true);
		const FVector& HoldLocation = GrabbedHold->GetActorLocation();
		const FVector& ActorLocation = GetActorLocation();
		float HoldDistance = FVector::Distance(HoldLocation, ActorLocation);
		if (HoldDistance > GrabDistanceTreshold)
		{
			GetCharacterMovement()->StopMovementImmediately();
			FVector NewLocation = FMath::VInterpTo(ActorLocation, HoldLocation, DeltaSeconds, 2.5f);
			SetActorLocation(NewLocation);
		}
	}
}

void AUE5TopDownARPGCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AUE5TopDownARPGCharacter, Health);
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
