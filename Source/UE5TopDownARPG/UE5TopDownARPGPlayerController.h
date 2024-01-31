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

	void OnActivateAbilityStarted();

	UFUNCTION()
	void OnMoveInputTriggered(const FInputActionInstance& Instance);

	UFUNCTION()
	void OnJumpInputTriggered(const FInputActionInstance& Instance);

	UFUNCTION()
	void OnClimbJumpTriggered(const FInputActionInstance& Instance);

	UFUNCTION()
	void OnClimbReleaseTriggered(const FInputActionInstance& Instance);

	UFUNCTION()
	void OnLookAtDirectionTriggered(const FInputActionInstance& Instance);

	UFUNCTION()
	void OnLookAtDirectionCanceled(const FInputActionInstance& Instance);

private:
	UFUNCTION()
	void OnCharacterHoldGrabbed(AActor* Hold);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* ClimbMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ActivateAbilityAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveInputAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpInputAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ClimbJumpInputAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ClimbReleaseInputAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAtInputAction;

	FVector ClimbLookDirection;
};


