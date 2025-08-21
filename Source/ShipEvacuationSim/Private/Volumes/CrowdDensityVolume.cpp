#include "Volumes/CrowdDensityVolume.h"
#include "Components/BoxComponent.h"
#include "NavModifierComponent.h"
#include "NavAreas/NavArea_Default.h"
#include "Volumes/CrowdedArea_NavArea.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

ACrowdDensityVolume::ACrowdDensityVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create and configure the volume component
	Volume = CreateDefaultSubobject<UBoxComponent>(TEXT("Volume"));
	RootComponent = Volume;
	Volume->SetBoxExtent(FVector(200.f, 200.f, 110.f));
	Volume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Volume->SetCollisionResponseToAllChannels(ECR_Ignore);
	Volume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// Create nav modifier and set default area
	NavModifier = CreateDefaultSubobject<UNavModifierComponent>(TEXT("NavModifier"));
	NavModifier->SetAreaClass(UNavArea_Default::StaticClass());
}

void ACrowdDensityVolume::BeginPlay()
{
	Super::BeginPlay();

	CheckCongestion();
	
	// Set a recurring timer to check congestion
	GetWorld()->GetTimerManager().SetTimer(
		DensityCheckTimerHandle,
		this,
		&ACrowdDensityVolume::CheckCongestion,
		CheckInterval, // Interval (in seconds) – reduce for faster reaction
		true  // Loop
	);
}

void ACrowdDensityVolume::CheckCongestion()
{
	TArray<AActor*> OverlappingActors;
	Volume->GetOverlappingActors(OverlappingActors, ACharacter::StaticClass());

	const int32 AgentCount = OverlappingActors.Num();
	if (AgentCount == 0)
	{
		if (bIsCongested)
		{
			bIsCongested = false;
			UpdateNavModifier(false);
			UE_LOG(LogTemp, Warning, TEXT("CrowdDensityVolume '%s': no agents, marked uncongested"), *GetName());
		}
		return;
	}

	// Agent capsule assumptions
	const float CapsuleRadius = 20.f;
	const float CapsuleArea = PI * FMath::Square(CapsuleRadius);
	const float OccupiedArea = AgentCount * CapsuleArea;

	// Volume area in 2D
	const FVector BoxExtent = Volume->GetScaledBoxExtent();
	const float TotalArea = (BoxExtent.X * 2.f) * (BoxExtent.Y * 2.f);
	const float AreaUsage = TotalArea > 0.f ? OccupiedArea / TotalArea : 0.f;

	// Movement check
	int32 SlowAgents = 0;
	for (AActor* Actor : OverlappingActors)
	{
		ACharacter* Character = Cast<ACharacter>(Actor);
		if (!Character || !Character->GetCharacterMovement()) continue;

		const float CurrentSpeed = Character->GetVelocity().Size();
		const float MaxSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;

		if (MaxSpeed <= 1.f || (CurrentSpeed / MaxSpeed) < SpeedSlowdownThreshold)
		{
			SlowAgents++;
		}
	}
	const float SlowRatio = static_cast<float>(SlowAgents) / AgentCount;

	const bool bDenseEnough = AreaUsage >= AreaOccupancyThreshold;
	const bool bSlowedEnough = SlowRatio >= PercentSlowedAgentsThreshold;

	const bool bNowCongested = bDenseEnough && bSlowedEnough;

	DisplayDebugStats(AreaUsage, SlowRatio, AgentCount);
	
	if (bNowCongested != bIsCongested)
	{
		bIsCongested = bNowCongested;
		UpdateNavModifier(bIsCongested);

		UE_LOG(LogTemp, Warning, TEXT("CrowdDensityVolume '%s' congestion changed: %s | Agents: %d | Area Usage: %.1f%% | Slow: %.1f%%"),
			*GetName(),
			bIsCongested ? TEXT("YES") : TEXT("NO"),
			AgentCount,
			AreaUsage * 100.f,
			SlowRatio * 100.f
		);
	}
}

void ACrowdDensityVolume::UpdateNavModifier(bool bCongested)
{
	if (bCongested)
	{
		// Apply high-cost area to discourage pathfinding through this volume
		NavModifier->SetAreaClass(UCrowdedArea_NavArea::StaticClass());
	}
	else
	{
		// Reset to normal navigation area
		NavModifier->SetAreaClass(UNavArea_Default::StaticClass());
	}
}

void ACrowdDensityVolume::DisplayDebugStats(float AreaUsage, float SlowRatio, int32 AgentCount)
{
	const FString DebugText = FString::Printf(
		TEXT("CrowdVolume '%s'\nCongested: %s\nAgents: %d\nArea: %.1f%%\nSlowed: %.1f%%"),
		*GetName(),
		bIsCongested ? TEXT("YES") : TEXT("NO"),
		AgentCount,
		AreaUsage * 100.f,
		SlowRatio * 100.f
	);

	DrawDebugString(
		GetWorld(),
		GetActorLocation(), // float above the volume
		DebugText,
		this,
		FColor::Yellow,
		5.f, // 0 = show one frame; will be redrawn each tick/timer
		true // center text
	);
}