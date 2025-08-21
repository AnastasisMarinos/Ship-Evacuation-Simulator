#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AICharacter.generated.h"

UCLASS()
class SHIPEVACUATIONSIM_API AAiCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AAiCharacter(const FObjectInitializer& ObjectInitializer);

	// Public States
	UPROPERTY(BlueprintReadWrite)
	bool bHasMustered = false;

	UPROPERTY(BlueprintReadWrite)
	float WalkSpeedOnFlat = 150.0f;

	UPROPERTY(BlueprintReadWrite)
	float WalkSpeedOnStairs = 100.0f;

	// Core Events
	virtual void Tick(float DeltaTime) override;

	// Avoidance Helpers
	void AdjustAvoidanceWeight(int32 NearbyAgents);
	int32 GetNearbyAgentsCount();
	TArray<AActor*> GetNearbyAgents();

	// Avoidance Modifiers
	UFUNCTION(BlueprintCallable)
	void RoomAvoidance();
    
	UFUNCTION(BlueprintCallable)
	void ResetAvoidance();

	// Capsule Size Logic
	void ShrinkCapsule();
	void RestoreCapsule();
	bool IsSafeToRestoreCapsule() const;

	// Utility Updates
	void ThrottledUpdate();
	void CheckIfStuck();
	float EstimateNavMeshWidth(float SampleDistance);

	// NavMesh Recovery
	void CheckNavMeshRecovery();
	FVector FindClosestValidPoint();
	void StartNavMeshRecoveryCheck();
	void SmoothRecoverToNavMesh(float DeltaTime);

	UFUNCTION(BlueprintCallable)
	void FinishedMustering();

	bool IsOnStairs() const;
	void ApplyDownhillNudge();

protected:
	virtual void BeginPlay() override;

private:
	// Avoidance
	float CustomAvoidanceWeight = 0.0f;
	TArray<AActor*> NearbyAgentsCache;

	// Stuck Detection
	FVector LastLocation;
	float TimeSinceLastMove = 0.0f;

	// Capsule Resize
	bool bCapsuleShrunk = false;
	bool bIsResizingCapsule = false;
	float DefaultCapsuleRadius = 0.0f;
	float DefaultCapsuleHalfHeight = 0.0f;
	float TargetCapsuleRadius = 0.0f;
	float TargetCapsuleHalfHeight = 0.0f;
	float ResizeSpeed = 50.0f; // Units/sec
	FTimerHandle CapsuleResetTimer;

	// NavMesh Recovery
	bool bIsRecovering = false;
	FVector RecoveryTargetLocation;
	float RecoverySpeed = 500.0f;
	FTimerHandle NavMeshCheckTimer;

	FTimerHandle ThrottledUpdateTimer;
	FTimerHandle StuckCheckTimer;
};
