// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "UE5TopDownARPGPlayerController.generated.h"

/** Forward declaration to improve compiling times */
class UNiagaraSystem;
class UInputAction;
class APawn;
class UEnhancedInputLocalPlayerSubsystem;
struct FInputActionInstance;

UCLASS()
class AUE5TopDownARPGPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void SetupInputComponent() override;
	// To add mapping context
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;

	void OnActivateAbilityStarted();

	UFUNCTION()
	void OnMoveInputTriggered(const FInputActionInstance& Instance);

	UFUNCTION()
	void OnJumpInputTriggered(const FInputActionInstance& Instance);

	UFUNCTION()
	void OnClimbJumpPressed(const FInputActionInstance& Instance);

	UFUNCTION()
	void OnClimbJumpReleased(const FInputActionInstance& Instance);

	UFUNCTION()
	void OnLookAtTriggered(const FInputActionInstance& Instance);

	UFUNCTION()
	void OnLookAtCompleted(const FInputActionInstance& Instance);

private:
	UFUNCTION()
	void OnCharacterHoldGrabbed(AActor* Hold);

	UFUNCTION()
	void OnCharacterHoldReleased(AActor* Hold);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Climb", meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* ClimbMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* ActivateAbilityAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveInputAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpInputAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Climb", meta = (AllowPrivateAccess = "true"))
	UInputAction* ClimbJumpInputAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Climb", meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAtInputAction;

	UPROPERTY()
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem;

	UPROPERTY(EditDefaultsOnly)
	float ClimbJumpMinForceFraction = 0.2f;
	/* The Direction used when doing a ClimbJump {(1,0) = Right; (0,1) = Up; (-1,0) = Left} */
	FVector2D ClimbJumpDirection;
	float ClimbJumpForceFraction = 0.2f;
	bool IsChargingJump = false;
};


