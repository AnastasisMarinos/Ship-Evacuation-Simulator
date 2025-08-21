// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentMovementComponent.h"

void UAgentMovementComponent::PhysWalking(float DeltaTime, int32 Iterations)
{
	Super::PhysWalking(DeltaTime, Iterations);

	const float MaxSpeed = GetMaxSpeed();

	FVector HorizontalVelocity = FVector(Velocity.X, Velocity.Y, 0.f);
	if (HorizontalVelocity.SizeSquared() > FMath::Square(MaxSpeed))
	{
		HorizontalVelocity = HorizontalVelocity.GetClampedToMaxSize(MaxSpeed);
		Velocity.X = HorizontalVelocity.X;
		Velocity.Y = HorizontalVelocity.Y;
	}
}