// Fill out your copyright notice in the Description page of Project Settings.

#include "SimulationManager.h"

#include "SimulationInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "TimerManager.h"


ASimulationManager::ASimulationManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ASimulationManager::BeginPlay()
{
    Super::BeginPlay();

    USimulationInstance* GameInstance = Cast<USimulationInstance>(GetGameInstance());
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("SimulationManager: GameInstance is not valid."));
        return;
    }

    RunIndex = GameInstance->PersistentRunIndex;

    LogDirectoryPath = FPaths::ProjectSavedDir() + TEXT("SimulationLogs/");
    IFileManager::Get().MakeDirectory(*LogDirectoryPath, true);

    CurrentSimFilePath = FString::Printf(TEXT("%sRun_%d.csv"), *LogDirectoryPath, RunIndex);

    FString Header = TEXT("Minute | AgentsMustered\n");
    FFileHelper::SaveStringToFile(Header, *CurrentSimFilePath);

    SimulationStartTime = FPlatformTime::Seconds();

    GetWorldTimerManager().SetTimer(MinuteLogTimer, this, &ASimulationManager::LogMinuteProgress, 60.0f, true);
    GetWorldTimerManager().SetTimer(SimulationTimeoutTimer, this, &ASimulationManager::EndCurrentSimulation, 1800.0f, false);
}

void ASimulationManager::LogMinuteProgress()
{
    int32 Mustered = CountMusteredAgents();
    FString Line = FString::Printf(TEXT("Minute: %d | Agents Mustered: %d\n"), MinutesElapsed++, Mustered);
    FFileHelper::SaveStringToFile(Line, *CurrentSimFilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
}

void ASimulationManager::EndCurrentSimulation()
{
    GetWorldTimerManager().ClearTimer(MinuteLogTimer);
    GetWorldTimerManager().ClearTimer(SimulationTimeoutTimer);

    double ElapsedSeconds = FPlatformTime::Seconds() - SimulationStartTime;
    FString Footer = FString::Printf(TEXT("TotalTimeSeconds,%.2f\n"), ElapsedSeconds);
    FFileHelper::SaveStringToFile(Footer, *CurrentSimFilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);

    USimulationInstance* GameInstance = Cast<USimulationInstance>(GetGameInstance());
    if (GameInstance)
    {
        GameInstance->PersistentRunIndex++;
    }

    if (RunIndex + 1 < TotalSimulations)
    {
        FString NextLevel = UGameplayStatics::GetCurrentLevelName(this, true);
        UGameplayStatics::OpenLevel(this, FName(*NextLevel)); // No need to pass SimIndex anymore
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("All simulations complete."));
        UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
    }
}

int32 ASimulationManager::CountMusteredAgents()
{
    return MusteredAgents;
}

