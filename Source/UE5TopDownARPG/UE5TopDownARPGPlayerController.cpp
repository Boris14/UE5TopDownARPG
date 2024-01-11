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
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
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