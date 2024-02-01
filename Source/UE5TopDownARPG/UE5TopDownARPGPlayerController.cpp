// Copyright Epic Games, Inc. All Rights Reserved.

#include "UE5TopDownARPGPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "UE5TopDownARPGCharacter.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PawnMovementComponent.h"
#include "UE5TopDownARPG.h"

void AUE5TopDownARPGPlayerController::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (IsValid(InputSubsystem) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("AUE5TopDownARPGPlayerController::BeginPlay IsValid(InputSubsystem) == false"));
		return;
	}
	InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
}

void AUE5TopDownARPGPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AUE5TopDownARPGCharacter* RPGCharacter = Cast<AUE5TopDownARPGCharacter>(InPawn);
	if (IsValid(RPGCharacter) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("AUE5TopDownARPGPlayerController::OnPossess IsValid(RPGCharacter) == false"));
		return;
	}

	TDelegate<void(AActor*)> OnHoldGrabbedDelegate;
	TDelegate<void(AActor*)> OnHoldReleasedDelegate;
	OnHoldGrabbedDelegate.BindUObject(this, &AUE5TopDownARPGPlayerController::OnCharacterHoldGrabbed);
	OnHoldReleasedDelegate.BindUObject(this, &AUE5TopDownARPGPlayerController::OnCharacterHoldReleased);
	RPGCharacter->SetOnHoldGrabbedDelegate(OnHoldGrabbedDelegate);
	RPGCharacter->SetOnHoldReleasedDelegate(OnHoldReleasedDelegate);
}

void AUE5TopDownARPGPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(MoveInputAction, ETriggerEvent::Triggered, this, &AUE5TopDownARPGPlayerController::OnMoveInputTriggered);
		EnhancedInputComponent->BindAction(JumpInputAction, ETriggerEvent::Triggered, this, &AUE5TopDownARPGPlayerController::OnJumpInputTriggered);
		EnhancedInputComponent->BindAction(ClimbJumpInputAction, ETriggerEvent::Triggered, this, &AUE5TopDownARPGPlayerController::OnClimbJumpTriggered);
		EnhancedInputComponent->BindAction(LookAtInputAction, ETriggerEvent::Triggered, this, &AUE5TopDownARPGPlayerController::OnLookAtTriggered);
		EnhancedInputComponent->BindAction(LookAtInputAction, ETriggerEvent::Completed, this, &AUE5TopDownARPGPlayerController::OnLookAtCompleted);
		EnhancedInputComponent->BindAction(ActivateAbilityAction, ETriggerEvent::Started, this, &AUE5TopDownARPGPlayerController::OnActivateAbilityStarted);
	}
}

void AUE5TopDownARPGPlayerController::OnActivateAbilityStarted()
{
	UE_LOG(LogUE5TopDownARPG, Log, TEXT("OnActivateAbilityStarted"));

	AUE5TopDownARPGCharacter* ARPGCharacter = Cast<AUE5TopDownARPGCharacter>(GetPawn());
	if (IsValid(ARPGCharacter))
	{
		FHitResult Hit;

		// If we hit a surface, cache the location
		if (GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit))
		{
			ARPGCharacter->ActivateAbility(Hit.Location);
		}
	}
}

void AUE5TopDownARPGPlayerController::OnMoveInputTriggered(const FInputActionInstance& Instance)
{
	const FInputActionValue& InputValue = Instance.GetValue();
	if (InputValue.GetValueType() != EInputActionValueType::Axis2D)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("AUE5TopDownARPGPlayerController::OnMoveInputTriggered InputValue.GetValueType() != EInputActionValueType::Axis2D"));
		return;
	}

	const FVector2D& Value = InputValue.Get<FVector2D>();
	FVector MoveDirection{ Value.Y, Value.X, 0.f };

	AUE5TopDownARPGCharacter* UECharacter = Cast<AUE5TopDownARPGCharacter>(GetPawn());
	if (IsValid(UECharacter) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("AUE5TopDownARPGPlayerController::OnMoveInputTriggered IsValid(UECharacter) == false"));
		return;
	}
	UECharacter->AddMovementInput(MoveDirection);
}

void AUE5TopDownARPGPlayerController::OnJumpInputTriggered(const FInputActionInstance& Instance)
{
	AUE5TopDownARPGCharacter* UECharacter = Cast<AUE5TopDownARPGCharacter>(GetPawn());
	if (IsValid(UECharacter) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("AUE5TopDownARPGPlayerController::OnJumpInputTriggered IsValid(UECharacter) == false"));
		return;
	}
	UECharacter->Jump();
}

void AUE5TopDownARPGPlayerController::OnClimbJumpTriggered(const FInputActionInstance& Instance)
{
	AUE5TopDownARPGCharacter* UECharacter = Cast<AUE5TopDownARPGCharacter>(GetPawn());
	if (IsValid(UECharacter) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("AUE5TopDownARPGPlayerController::OnClimbJumpTriggered IsValid(RPGCharacter) == false"));
		return;
	}

	UECharacter->ClimbJump(ClimbLookDirection);
}

void AUE5TopDownARPGPlayerController::OnLookAtTriggered(const FInputActionInstance& Instance)
{
	const FVector& Value = Instance.GetValue().Get<FVector>();
	ClimbLookDirection = FVector2D{ Value.Y - Value.X, Value.Z };
}

void AUE5TopDownARPGPlayerController::OnLookAtCompleted(const FInputActionInstance& Instance)
{
 	ClimbLookDirection = FVector2D::ZeroVector;
}

void AUE5TopDownARPGPlayerController::OnCharacterHoldGrabbed(AActor* Hold)
{
	if (IsValid(Hold) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("AUE5TopDownARPGPlayerController::OnCharacterHoldGrabbed IsValid(Hold) == false"));
		return;
	}
	if (IsValid(InputSubsystem) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("AUE5TopDownARPGPlayerController::OnCharacterHoldGrabbed IsValid(InputSubsystem) == false"));
		return;
	}
	InputSubsystem->RemoveMappingContext(DefaultMappingContext);
	InputSubsystem->AddMappingContext(ClimbMappingContext, 0);
}

void AUE5TopDownARPGPlayerController::OnCharacterHoldReleased(AActor* Hold)
{
	if (IsValid(Hold) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("AUE5TopDownARPGPlayerController::OnCharacterHoldReleased IsValid(Hold) == false"));
		return;
	}
	if (IsValid(InputSubsystem) == false)
	{
		UE_LOG(LogUE5TopDownARPG, Error, TEXT("AUE5TopDownARPGPlayerController::OnCharacterHoldReleased IsValid(InputSubsystem) == false"));
		return;
	}
	InputSubsystem->RemoveMappingContext(ClimbMappingContext);
	InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
}
