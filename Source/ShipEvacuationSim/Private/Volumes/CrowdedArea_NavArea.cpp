// Fill out your copyright notice in the Description page of Project Settings.


#include "Volumes/CrowdedArea_NavArea.h"

UCrowdedArea_NavArea::UCrowdedArea_NavArea()
{
	DefaultCost = 1.0f;
	FixedAreaEnteringCost = 3000.0f;    // Discourage entering from outside
	DrawColor = FColor::Red;
}