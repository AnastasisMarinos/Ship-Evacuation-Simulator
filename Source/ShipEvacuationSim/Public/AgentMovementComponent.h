// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AgentMovementComponent.generated.h"


UCLASS()
class SHIPEVACUATIONSIM_API UAgentMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

protected:
	virtual void PhysWalking(float DeltaTime, int32 Iterations) override;
};
