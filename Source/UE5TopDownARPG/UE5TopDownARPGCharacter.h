// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "UE5TopDownARPGCharacter.generated.h"

class UBoxComponent;
class UArrowComponent;
class UCameraComponent;
class USpringArmComponent;
class UBehaviorTree;
class UBaseAbility;

UCLASS(Blueprintable)
class AUE5TopDownARPGCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AUE5TopDownARPGCharacter();

	// Called every frame.
	virtual void BeginPlay() override;

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void SetIsJumpArrowVisible(bool IsVisible);

	UFUNCTION()
	void UpdateJumpArrow(const FVector2D& Direction, float MaxLengthFraction);

	UFUNCTION()
	void ClimbJump(FVector2D InDirection, float MaxForceFraction);

	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	FORCEINLINE class UBehaviorTree* GetBehaviorTree() const { return BehaviorTree; }

	void SetOnHoldGrabbedDelegate(const TDelegate<void(AActor*)>& InOnHoldGrabbedDelegate) { OnHoldGrabbedDelegate = InOnHoldGrabbedDelegate; }
	void SetOnHoldReleasedDelegate(const TDelegate<void(AActor*)>& InOnHoldReleasedDelegate) { OnHoldReleasedDelegate = InOnHoldReleasedDelegate; }

	bool ActivateAbility(FVector Location);

private:
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* TopDownCameraComponent;

	/* The Area where the Character can grab Holds */
	UPROPERTY(EditDefaultsOnly, Category = Climbing)
	UBoxComponent* ClimbingBoxComponent;

	/* Arrow Component showing the direction and charge when doing a ClimbJump */
	UPROPERTY(EditDefaultsOnly, Category = Climbing)
	UArrowComponent* ClimbJumpArrowComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(EditDefaultsOnly)
	UBehaviorTree* BehaviorTree;

	UPROPERTY()
	UBaseAbility* AbilityInstance;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UBaseAbility> AbilityTemplate;

	UPROPERTY(ReplicatedUsing = OnRep_SetHealth, EditDefaultsOnly)
	float Health = 100.0f;

	UPROPERTY(EditDefaultsOnly)
	float DeathDelay = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = Climbing)
	FName GrabSocketName = "Grab";
	UPROPERTY(EditDefaultsOnly, Category = Climbing)
	FName LeftHandSocketName = "Left Hand";
	UPROPERTY(EditDefaultsOnly, Category = Climbing)
	FName RightHandSocketName = "Right Hand";
	/* A Tag used to check which Actors are Climbing Holds */
	UPROPERTY(EditDefaultsOnly, Category = Climbing)
	FName ClimbingHoldsActorTag;
	UPROPERTY(EditDefaultsOnly, Category = Climbing)
	float ClimbJumpArrowMaxLength = 200.f;
	UPROPERTY(EditDefaultsOnly, Category = Climbing)
	float GrabDistanceTreshold = 80.f;
	UPROPERTY(EditDefaultsOnly, Category = Climbing)
	float ClimbJumpMaxForce = 1100.f;
	/* The force at which you pull towards a Hold when you grab it */
	UPROPERTY(EditDefaultsOnly, Category = Climbing)
	float PullToHoldForce = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = Camera)
	float OriginalCameraPitch = -50.f;
	UPROPERTY(EditDefaultsOnly, Category = Camera)
	float OriginalCameraDistance = 1400.f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Climbing")
	float ClimbCameraPitch = -20.f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Climbing")
	float ClimbCameraDistance = 2100.f;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AActor> AfterDeathSpawnClass;

	UPROPERTY()
	AActor* GrabbedHold = nullptr;

	TDelegate<void(AActor*)> OnHoldGrabbedDelegate;
	TDelegate<void(AActor*)> OnHoldReleasedDelegate;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FVector IKOffsetRightHand;
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FVector IKOffsetLeftHand;
	FRotator DesiredRotation;
	FTimerHandle DeathHandle;

	UFUNCTION()
	void TakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigateBy, AActor* DamageCauser);

	UFUNCTION()
	void OnClimbingComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, 
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_SetHealth(float OldHealth);

	UFUNCTION()
	void GrabHold(AActor* Hold, const FVector& OverlapLocation);

	UFUNCTION()
	void ReleaseHold();

	UFUNCTION()
	FRotator GetFaceWallRotation() const;

	void Death();
};

