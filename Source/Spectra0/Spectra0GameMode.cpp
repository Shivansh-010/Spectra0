#include "Spectra0GameMode.h"
// #include "Terminal.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

void ASpectra0GameMode::BeginPlay()
{
	Super::BeginPlay();

	ExecutionManager = NewObject<UExecutionManager>(this);
	if (ExecutionManager)
	{
		ExecutionManager->Initialize(GetWorld());
		UE_LOG(LogTemp, Log, TEXT("Execution Manager initialized"));
	}
//
//	// Log to confirm BeginPlay is called
//	UE_LOG(LogTemp, Log, TEXT("GameMode BeginPlay: Spawning Terminal"));
//
//	if (GetWorld())
//	{
//		// Spawn parameters
//		FActorSpawnParameters SpawnParams;
//		SpawnParams.Name = TEXT("TerminalInstance");
//
//		// Spawn location and rotation (set to origin for now)
//		FVector Location(0.0f, 0.0f, 0.0f);
//		FRotator Rotation(0.0f, 0.0f, 0.0f);
//
//		// Spawn the terminal actor
//		ATerminal* Terminal = GetWorld()->SpawnActor<ATerminal>(ATerminal::StaticClass(), Location, Rotation, SpawnParams);
//
//		if (Terminal)
//		{
//			UE_LOG(LogTemp, Log, TEXT("Terminal Actor spawned successfully"));
//			// You can call terminal methods here, e.g.
//			// Terminal->SetRootAccessEnabled(true);
//		}
//		else
//		{
//			UE_LOG(LogTemp, Error, TEXT("Failed to spawn Terminal Actor"));
//		}
//	}
}
