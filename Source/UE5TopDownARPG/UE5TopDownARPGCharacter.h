// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "UE5TopDownARPGCharacter.generated.h"

class USphereComponent;
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

	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	FORCEINLINE class UBehaviorTree* GetBehaviorTree() const { return BehaviorTree; }

	bool ActivateAbility(FVector Location);

private:
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* TopDownCameraComponent;

	UPROPERTY(EditDefaultsOnly, Category = Climbing)
	USphereComponent* ClimbingSphereComponent;

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
	/* A Tag used to check which Actors are Climbing Holds */
	FName ClimbingHoldsActorTag;

	FTimerHandle DeathHandle;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AActor> AfterDeathSpawnClass;

	UFUNCTION()
	void TakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigateBy, AActor* DamageCauser);

	UFUNCTION()
	void OnClimbingComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, 
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_SetHealth(float OldHealth);

	void GrabHold(AActor* Hold, const FVector& OverlapLocation);

	void Death();
};

