// FireArea_NavArea.h
#pragma once

#include "CoreMinimal.h"
#include "NavAreas/NavArea.h"
#include "FireArea_NavArea.generated.h"

UCLASS()
class UFireArea_NavArea : public UNavArea
{
	GENERATED_BODY()

public:
	UFireArea_NavArea()
	{
		DefaultCost = 5000.f; // High cost to enter
		FixedAreaEnteringCost = 10000.f; // Even higher cost to discourage entry
		DrawColor = FColor::Red;
	}
};