// FireArea_NavArea.h
#pragma once

#include "CoreMinimal.h"
#include "NavAreas/NavArea.h"
#include "SmokeArea_NavArea.generated.h"

UCLASS()
class USmokeArea_NavArea : public UNavArea
{
	GENERATED_BODY()

public:
	
	USmokeArea_NavArea()
	{
		DefaultCost = 5000.f; // High cost to enter
		FixedAreaEnteringCost = 10000.f; // Even higher cost to discourage entry
		DrawColor = FColor::Red;
	}
};