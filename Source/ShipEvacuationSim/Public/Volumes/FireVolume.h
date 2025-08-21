#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FireVolume.generated.h"

class UBoxComponent;
class UNavModifierComponent;

UCLASS()
class SHIPEVACUATIONSIM_API AFireVolume : public AActor
{
	GENERATED_BODY()

public:
	AFireVolume();

protected:
	virtual void BeginPlay() override;

public:
	// Collision volume that expands over time
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fire Volume")
	UBoxComponent* FireBox;

	// Navigation modifier to block pathfinding entry
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fire Volume")
	UNavModifierComponent* NavModifier;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
	UStaticMeshComponent* SmokeVisualBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire Volume", meta = (ClampMin = "0.1"))
	float UpdateInterval = 0.2f;

	// Time in seconds to reach max size
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire Volume", meta = (ClampMin = "0.1"))
	float ExpansionDuration = 480.0f;

	// Max size in X and Y (Z remains fixed)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire Volume")
	FVector2D MaxXYSize = FVector2D(5000.f, 5000.f);

private:
	float ElapsedTime = 0.0f;
	FVector InitialExtent;

	FTimerHandle ExpansionTimerHandle;

	void ExpandVolumeStep();
};
