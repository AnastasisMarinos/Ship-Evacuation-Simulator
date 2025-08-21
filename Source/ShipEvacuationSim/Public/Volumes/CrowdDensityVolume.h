#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CrowdDensityVolume.generated.h"

class UNavModifierComponent;
class UBoxComponent;

/**
 * Volume that monitors AI crowd density and movement slowdown to mark itself congested.
 * It modifies the navmesh cost through a NavModifierComponent when congested.
 */
UCLASS()
class SHIPEVACUATIONSIM_API ACrowdDensityVolume : public AActor
{
	GENERATED_BODY()

public:
	ACrowdDensityVolume();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crowd")
	UBoxComponent* Volume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	UNavModifierComponent* NavModifier;

	// Tuning Parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "")
	float AreaOccupancyThreshold = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "")
	float SpeedSlowdownThreshold = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "")
	float PercentSlowedAgentsThreshold = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "")
	float CheckInterval = 5.0f;

protected:
	virtual void BeginPlay() override;

private:
	bool bIsCongested = false;
	FTimerHandle DensityCheckTimerHandle;

	void CheckCongestion();
	void UpdateNavModifier(bool bCongested);

	void DisplayDebugStats(float AreaUsage, float SlowRatio, int32 AgentCount);
};
