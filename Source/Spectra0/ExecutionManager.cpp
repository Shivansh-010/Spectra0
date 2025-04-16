// Fill out your copyright notice in the Description page of Project Settings.

#include "ExecutionManager.h"
#include "Terminal.h"
#include "QueryStepperModel.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

void UExecutionManager::Initialize(UWorld* WorldContext)
{
	// Spawn terminal actor
	if (WorldContext)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Name = TEXT("SpectraTerminal");

		Terminal = WorldContext->SpawnActor<ATerminal>(ATerminal::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (Terminal)
		{
			UE_LOG(LogTemp, Log, TEXT("Terminal spawned by ExecutionManager"));
		}
	}

	// Create and initialize query stepper model
	Stepper = NewObject<UQueryStepperModel>(this);
	if (Stepper)
	{
		Stepper->OnResponseReceived.AddDynamic(this, &UExecutionManager::OnModelResponse);
		Stepper->Initialize();

		// Send an initial prompt
		Stepper->SendMessage(TEXT("Respond only with a smile emoji"));
	}
}

void UExecutionManager::HandleTerminalOutput(const FString& Output)
{
	if (Stepper)
	{
		Stepper->SendMessage(Output);
	}
}

void UExecutionManager::OnModelResponse(const FString& Response)
{
	// UE_LOG(LogTemp, Log, TEXT("Model responded: %s"), *Response);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Red, FString::Printf(TEXT("Model: %s"), *Response));
	}

	// TODO: Route to next model or terminal if needed
}


