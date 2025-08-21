#include "AICharacter.h"
#include "AgentMovementComponent.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

// Constructor
AAiCharacter::AAiCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UAgentMovementComponent>(CharacterMovementComponentName))
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = FMath::FRandRange(0.2f, 0.5f);
    
    GetCharacterMovement()->bUseRVOAvoidance = true;
}

// BeginPlay
void AAiCharacter::BeginPlay()
{
    Super::BeginPlay();

    float ThrotleRandomOffset = FMath::FRandRange(0.25f, 0.35f);
    GetWorldTimerManager().SetTimer(ThrottledUpdateTimer, this, &AAiCharacter::ThrottledUpdate, ThrotleRandomOffset, true);
    float StuckRandomOffset = FMath::FRandRange(2.0f, 3.5f);
    GetWorldTimerManager().SetTimer(StuckCheckTimer, this, &AAiCharacter::CheckIfStuck, StuckRandomOffset, true);

    // Initial Avoidance setup
    GetCharacterMovement()->AvoidanceConsiderationRadius = 50;

    // Cache capsule size
    DefaultCapsuleRadius = GetCapsuleComponent()->GetUnscaledCapsuleRadius();
    DefaultCapsuleHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
    LastLocation = GetActorLocation();

    StartNavMeshRecoveryCheck();
}

// Tick
void AAiCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Repulsion behavior
    FVector RepulsionForce = FVector::ZeroVector;
    for (AActor* Other : NearbyAgentsCache)
    {
        if (Other == this) continue;

        FVector ToOther = GetActorLocation() - Other->GetActorLocation();
        float Distance = ToOther.Size();
        if (Distance < 80.0f && Distance > KINDA_SMALL_NUMBER)
        {
            float Strength = (20.0f - Distance) / 20.0f;
            RepulsionForce += ToOther.GetSafeNormal() * Strength;
        }
    }

    RepulsionForce = RepulsionForce.GetClampedToMaxSize(1.0f);
    
    FVector VelocityNorm = GetVelocity().GetSafeNormal();
    bool bHasMovement = !GetVelocity().IsNearlyZero();

    FVector RepulsionForceClamped = RepulsionForce.GetClampedToMaxSize(1.0f);
    FVector FinalDirection = (bHasMovement ? VelocityNorm : FVector::ZeroVector) + RepulsionForceClamped * 2.0f;

    FinalDirection = FinalDirection.GetSafeNormal();

    // Only add movement input if there's a meaningful direction
    if (!FinalDirection.IsNearlyZero())
    {
        // Optional: only move if repulsion is strong or agent is already moving
        if (bHasMovement || RepulsionForceClamped.SizeSquared() > 0.01f)
        {
            AddMovementInput(FinalDirection, 1.0f);
        }
    }

    // Capsule resizing interpolation
    if (bIsResizingCapsule)
    {
        float CurrentRadius = GetCapsuleComponent()->GetUnscaledCapsuleRadius();
        float CurrentHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
        float NewRadius = FMath::FInterpTo(CurrentRadius, TargetCapsuleRadius, DeltaTime, ResizeSpeed);
        float NewHalfHeight = FMath::FInterpTo(CurrentHalfHeight, TargetCapsuleHalfHeight, DeltaTime, ResizeSpeed);

        GetCapsuleComponent()->SetCapsuleSize(NewRadius, NewHalfHeight, true);

        if (FMath::IsNearlyEqual(NewRadius, TargetCapsuleRadius, 0.5f) &&
            FMath::IsNearlyEqual(NewHalfHeight, TargetCapsuleHalfHeight, 0.5f))
        {
            bIsResizingCapsule = false;
        }
    }

    if (bIsRecovering)
    {
        SmoothRecoverToNavMesh(DeltaTime);
    }
}

// Capsule Resizing
void AAiCharacter::ShrinkCapsule()
{
    if (!bHasMustered)
    {
        bCapsuleShrunk = true;
        bIsResizingCapsule = true;

        TargetCapsuleRadius = DefaultCapsuleRadius * 0.7f;
        TargetCapsuleHalfHeight = DefaultCapsuleHalfHeight * 0.8f;

        GetWorldTimerManager().SetTimer(CapsuleResetTimer, this, &AAiCharacter::RestoreCapsule, 2.0f, false);
    }
}

void AAiCharacter::RestoreCapsule()
{
    bIsResizingCapsule = true;
    bCapsuleShrunk = false;

    TargetCapsuleRadius = DefaultCapsuleRadius;
    TargetCapsuleHalfHeight = DefaultCapsuleHalfHeight;
    TimeSinceLastMove = 0.0f;
}

// Updates
void AAiCharacter::ThrottledUpdate()
{
    // Adjust movement speed based on vertical velocity
    float VerticalSpeed = GetVelocity().Z;
    GetCharacterMovement()->MaxWalkSpeed = FMath::Abs(VerticalSpeed) > 20.0f ? WalkSpeedOnStairs : WalkSpeedOnFlat;
    
    float NavWidth = EstimateNavMeshWidth(50.0f);

    // In tight spaces (width ≈ 80), we want high avoidance weight (e.g. 30)
    // In open spaces (width ≈ 200+), we want low avoidance weight (e.g. 10)
    float ScaledWeight = FMath::GetMappedRangeValueClamped(
        FVector2D(200.0f, 80.0f),     // Inverted input range
        FVector2D(10.0f, 30.0f),      // Output: more avoidance in narrow areas
        NavWidth
    );

    GetCharacterMovement()->AvoidanceWeight = ScaledWeight;

    NearbyAgentsCache = GetNearbyAgents();
    //AdjustAvoidanceWeight(NearbyAgentsCache.Num());
}

void AAiCharacter::CheckIfStuck()
{
    float DistanceMoved = FVector::Dist(GetActorLocation(), LastLocation);

    if (DistanceMoved < 5.0f)
    {
        TimeSinceLastMove += 0.3f;
    }
    else
    {
        TimeSinceLastMove = 0.0f;
        LastLocation = GetActorLocation();
    }

    if (TimeSinceLastMove > 2.0f && !bCapsuleShrunk)
    {
        ShrinkCapsule();

        if (IsOnStairs())
        {
            ApplyDownhillNudge();
        }
    }
}

// Avoidance Helpers
TArray<AActor*> AAiCharacter::GetNearbyAgents()
{
    TArray<AActor*> NearbyAgents;
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->GetOverlappingActors(NearbyAgents, AAiCharacter::StaticClass());
    }
    return NearbyAgents;
}

int32 AAiCharacter::GetNearbyAgentsCount()
{
    TArray<AActor*> OverlappingActors;
    GetCapsuleComponent()->GetOverlappingActors(OverlappingActors);

    return OverlappingActors.FilterByPredicate([](AActor* Actor)
    {
        return Actor->IsA(AAiCharacter::StaticClass());
    }).Num();
}

void AAiCharacter::AdjustAvoidanceWeight(int32 NearbyAgents)
{
    float MaxWeight = 15.0f;
    float MinWeight = 10.0f;
    float BaseWeight = 10.0f;

    CustomAvoidanceWeight = FMath::Clamp(BaseWeight - (NearbyAgents * 0.2f), MinWeight, MaxWeight);
    GetCharacterMovement()->AvoidanceWeight = CustomAvoidanceWeight;
}

// NavMesh Recovery
void AAiCharacter::CheckNavMeshRecovery()
{
    if (bIsRecovering) return;

    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if (!NavSys) return;

    FNavLocation ProjectedLocation;
    if (!NavSys->ProjectPointToNavigation(GetActorLocation(), ProjectedLocation, FVector(100.f)))
    {
        UE_LOG(LogTemp, Warning, TEXT("Agent %s is off the navmesh."), *GetName());
    
        FVector Fallback = FindClosestValidPoint(); // <- See below for implementation
        RecoveryTargetLocation = Fallback;
        bIsRecovering = true;

        GetCharacterMovement()->DisableMovement();
    }
}

FVector AAiCharacter::FindClosestValidPoint()
{
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if (!NavSys) return GetActorLocation(); // fail-safe

    FNavLocation ValidLocation;
    bool bFound = NavSys->ProjectPointToNavigation(GetActorLocation(), ValidLocation, FVector(500.f)); // bigger radius

    return bFound ? ValidLocation.Location : GetActorLocation(); // safer fallback
}

void AAiCharacter::StartNavMeshRecoveryCheck()
{
    float RandomOffset = FMath::FRandRange(3.0f, 4.0f);
    GetWorldTimerManager().SetTimer(NavMeshCheckTimer, this, &AAiCharacter::CheckNavMeshRecovery, RandomOffset, true);
}

void AAiCharacter::SmoothRecoverToNavMesh(float DeltaTime)
{
    FVector NewLocation = FMath::VInterpTo(GetActorLocation(), RecoveryTargetLocation, DeltaTime, 2.0f);
    if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld()))
    {
        FNavLocation NavLoc;
        if (NavSys->ProjectPointToNavigation(NewLocation, NavLoc, FVector(50.f)))
        {
            SetActorLocation(NavLoc.Location);
        }
    }

    if (FVector::DistSquared(GetActorLocation(), RecoveryTargetLocation) < 100.0f)
    {
        bIsRecovering = false;

        UCharacterMovementComponent* MoveComp = GetCharacterMovement();
        if (MoveComp)
        {
            MoveComp->SetMovementMode(MOVE_Walking);
            MoveComp->Velocity = FVector::ZeroVector; // Reset lingering bad velocity
            MoveComp->UpdateComponentVelocity();      // Force update
        }
    }
}

void AAiCharacter::FinishedMustering()
{
    // 1. Stop ticking
    SetActorTickEnabled(false);

    // 2. Clear all timers
    GetWorldTimerManager().ClearAllTimersForObject(this);

    // 3. Stop movement
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->DisableMovement();
    }

    // 4. Stop behavior tree
    AAIController* AIController = Cast<AAIController>(GetController());
    if (AIController)
    {
        AIController->UnPossess();
    }
}

// Optional External Triggers
void AAiCharacter::RoomAvoidance()
{
    GetCharacterMovement()->AvoidanceConsiderationRadius = 20.0f;
}

void AAiCharacter::ResetAvoidance()
{
    GetCharacterMovement()->AvoidanceConsiderationRadius = 20.0f;
}

// NavMesh Width Estimation
float AAiCharacter::EstimateNavMeshWidth(float SampleDistance)
{
    FVector Location = GetActorLocation();
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if (!NavSys) return 0.f;

    const FVector LeftOffset = GetActorRightVector() * -SampleDistance;
    const FVector RightOffset = GetActorRightVector() * SampleDistance;

    FNavLocation LeftPoint, RightPoint;
    bool bLeft = NavSys->ProjectPointToNavigation(Location + LeftOffset, LeftPoint);
    bool bRight = NavSys->ProjectPointToNavigation(Location + RightOffset, RightPoint);

    if (bLeft && bRight)
        return FVector::Dist(LeftPoint.Location, RightPoint.Location);
    if (bLeft || bRight)
        return SampleDistance * 2.0f;

    return 0.f;
}

bool AAiCharacter::IsOnStairs() const
{
    const FFindFloorResult& FloorResult = GetCharacterMovement()->CurrentFloor;
    if (!FloorResult.bBlockingHit)
        return false;

    float FloorAngleDegrees = FMath::RadiansToDegrees(FMath::Acos(FloorResult.HitResult.Normal.Z));
    return FloorAngleDegrees > 10.0f && FloorAngleDegrees < 45.0f; // Tune for your stairs
}

void AAiCharacter::ApplyDownhillNudge()
{
    FVector Forward = GetActorForwardVector();
    FVector Nudge = (Forward - FVector(0, 0, 0.2f)).GetSafeNormal() * 100.0f;

    GetCharacterMovement()->Velocity += Nudge;
}
