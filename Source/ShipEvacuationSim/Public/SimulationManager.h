// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SimulationManager.generated.h"

UCLASS()
class SHIPEVACUATIONSIM_API ASimulationManager : public AActor
{
	GENERATED_BODY()

public:
	ASimulationManager();
	
	UPROPERTY(BlueprintReadWrite, Category = "Simulation")
	int32 MusteredAgents;
	
protected:
	virtual void BeginPlay() override;

private:
	int32 RunIndex = 0;
	int32 TotalSimulations = 50;
	int32 MinutesElapsed = 1;

	FString LogDirectoryPath;
	FString CurrentSimFilePath;

	FTimerHandle MinuteLogTimer;
	FTimerHandle SimulationTimeoutTimer;

	double SimulationStartTime = 0.0;

	void LogMinuteProgress();
	void EndCurrentSimulation();

	int32 CountMusteredAgents();
};
