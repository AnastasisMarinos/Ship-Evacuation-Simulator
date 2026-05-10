// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SimulationInstance.generated.h"


UCLASS()
class SHIPEVACUATIONSIM_API USimulationInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 PersistentRunIndex = 0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Simulation")
	TArray<float> ErrorPercentages{
		
		};
	
	UFUNCTION(BlueprintCallable, Category = "Simulation")
	float GetCurrentErrorPercentage() const
	{
		if (ErrorPercentages.Num() == 0) return 0.f;

		const int32 Index = FMath::Clamp(PersistentRunIndex, 0, ErrorPercentages.Num() - 1);
		return ErrorPercentages[Index];
	}
};
