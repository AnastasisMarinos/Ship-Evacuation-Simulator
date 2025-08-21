#include "Volumes/FireVolume.h"
#include "NavigationSystem.h"
#include "Components/BoxComponent.h"
#include "NavModifierComponent.h"
#include "Volumes/FireArea_NavArea.h"

AFireVolume::AFireVolume()
{
	PrimaryActorTick.bCanEverTick = true;

	// Fire volume collision
	FireBox = CreateDefaultSubobject<UBoxComponent>(TEXT("FireBox"));
	SetRootComponent(FireBox);
	FireBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FireBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	FireBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	FireBox->SetBoxExtent(FVector(25.f, 25.f, 110.f));
	FireBox->SetCanEverAffectNavigation(true);
	FireBox->bDynamicObstacle = true;
	FireBox->SetMobility(EComponentMobility::Movable);

	// Smoke visual mesh (cube)
	SmokeVisualBox = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SmokeVisualBox"));
	SmokeVisualBox->SetupAttachment(FireBox);
	SmokeVisualBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SmokeVisualBox->SetMobility(EComponentMobility::Movable);
	SmokeVisualBox->SetRelativeLocation(FVector::ZeroVector);

	// Set basic cube mesh (engine default)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		SmokeVisualBox->SetStaticMesh(CubeMesh.Object);
		SmokeVisualBox->SetWorldScale3D(FVector(0.5f)); // Placeholder until BeginPlay
	}

	// Nav modifier that blocks entry
	NavModifier = CreateDefaultSubobject<UNavModifierComponent>(TEXT("NavModifier"));
	NavModifier->SetAreaClass(UFireArea_NavArea::StaticClass());

	InitialExtent = FireBox->GetUnscaledBoxExtent();
}

void AFireVolume::BeginPlay()
{
	Super::BeginPlay();

	// Start the timer-based expansion
	GetWorld()->GetTimerManager().SetTimer(
		ExpansionTimerHandle,
		this,
		&AFireVolume::ExpandVolumeStep,
		UpdateInterval,
		true
	);

	// Force initial nav update
	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		NavSys->UpdateComponentInNavOctree(*FireBox);
	}
}

void AFireVolume::ExpandVolumeStep()
{
	if (ElapsedTime >= ExpansionDuration)
	{
		GetWorld()->GetTimerManager().ClearTimer(ExpansionTimerHandle);
		return;
	}

	ElapsedTime += UpdateInterval;

	const float Alpha = FMath::Clamp(ElapsedTime / ExpansionDuration, 0.f, 1.f);

	// ✅ Exponential growth curve: grows very slowly at first, then rapidly
	const float ExpFactor = FMath::Pow(Alpha, 4.0f); // Try 3.0–5.0 for tuning

	const FVector NewExtent = FVector(
		FMath::Lerp(InitialExtent.X, MaxXYSize.X / 2.f, ExpFactor),
		FMath::Lerp(InitialExtent.Y, MaxXYSize.Y / 2.f, ExpFactor),
		InitialExtent.Z
	);

	FireBox->SetBoxExtent(NewExtent);
	SmokeVisualBox->SetWorldScale3D((NewExtent * 2.0f) / 100.0f);
	FireBox->UpdateBounds();

	// Force navmesh update
	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		NavSys->UpdateComponentInNavOctree(*FireBox);
	}
}
