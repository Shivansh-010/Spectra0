// ExecutionManager.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ExecutionManager.generated.h"

class ATerminal;
class UQueryStepperModel;
class UAIModel;
class UTaggerModel;
class UCommandGeneratorModel;
class UCommandConsolidatorModel;
class UCommandVerifierModel;

UCLASS(Blueprintable)
class SPECTRA0_API UExecutionManager : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Execution")
	void Initialize(UWorld* WorldContext);

	void HandleTerminalOutput(const FString& Output);
	
	UFUNCTION()
	void OnModelResponse(const FString& Response);

	UFUNCTION(BlueprintCallable, Category = "AI Pipeline")
	void SendPromptToPipeline(const FString& Prompt, bool bRootAllowed);

	bool bRequestedRoot = false;

private:
	UPROPERTY()
	ATerminal* Terminal;

	UPROPERTY()
	UQueryStepperModel* Stepper;

	void ConfigureModelsFromFile(const FString& ConfigPath);
	
	// Add to private section
	UAIModel* LastRespondingModel = nullptr;

	UPROPERTY() UTaggerModel* Tagger;
	UPROPERTY() UCommandGeneratorModel* Generator;
	UPROPERTY() UCommandConsolidatorModel* Consolidator;
	UPROPERTY() UCommandVerifierModel* Verifier;
	
};