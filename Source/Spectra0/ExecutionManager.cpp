// Fill out your copyright notice in the Description page of Project Settings.

#include "ExecutionManager.h"

#include "CommandConsolidatorModel.h"
#include "CommandGeneratorModel.h"
#include "CommandVerifierModel.h"
#include "Terminal.h"
#include "QueryStepperModel.h"
#include "TaggerModel.h"
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

	// Create and attach models
	Stepper = NewObject<UQueryStepperModel>(this);
	if (Stepper)
	{
		Stepper->OnResponseReceived.AddDynamic(this, &UExecutionManager::OnModelResponse);
	}

	Tagger = NewObject<UTaggerModel>(this);
	if (Tagger)
	{
		Tagger->OnResponseReceived.AddDynamic(this, &UExecutionManager::OnModelResponse);
	}

	Generator = NewObject<UCommandGeneratorModel>(this);
	if (Generator)
	{
		Generator->OnResponseReceived.AddDynamic(this, &UExecutionManager::OnModelResponse);
	}

	Consolidator = NewObject<UCommandConsolidatorModel>(this);
	if (Consolidator)
	{
		Consolidator->OnResponseReceived.AddDynamic(this, &UExecutionManager::OnModelResponse);
	}

	Verifier = NewObject<UCommandVerifierModel>(this);
	if (Verifier)
	{
		Verifier->OnResponseReceived.AddDynamic(this, &UExecutionManager::OnModelResponse);
	}

	// Load model settings (API key, context, etc.)
	ConfigureModelsFromFile("D:/Obsidian_live/_KnowledgeBase/DataFiles/ModelConfig.md");

	LastRespondingModel = Stepper;
}

void UExecutionManager::ConfigureModelsFromFile(const FString& ConfigPath)
{
    FString ConfigText;
    if (!FFileHelper::LoadFileToString(ConfigText, *ConfigPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to read config file: %s"), *ConfigPath);
        return;
    }

    TMap<FString, TMap<FString, FString>> ModelConfigs; // ModelName -> (Key -> Value)
    FString CurrentModel;

    TArray<FString> Lines;
    ConfigText.ParseIntoArrayLines(Lines);

    for (const FString& Line : Lines)
    {
        if (Line.StartsWith("## ")) // Section header
        {
            CurrentModel = Line.Mid(3).TrimStartAndEnd();
            ModelConfigs.Add(CurrentModel, TMap<FString, FString>());
            UE_LOG(LogTemp, Log, TEXT("Parsing model config: %s"), *CurrentModel);
        }
        else if (Line.Contains(":") && !CurrentModel.IsEmpty())
        {
            FString Key, Value;
            if (Line.Split(":", &Key, &Value))
            {
                Key = Key.TrimStartAndEnd();
                Value = Value.TrimStartAndEnd();
                ModelConfigs[CurrentModel].Add(Key, Value);
                UE_LOG(LogTemp, Log, TEXT("  %s = %s"), *Key, *Value);
            }
        }
    }

    auto ConfigureModel = [](UAIModel* Model, const TMap<FString, FString>& Settings)
    {
        if (!Model) return;

        FString APIKey = Settings.Contains("APIKey") ? Settings["APIKey"] : TEXT("");
        FString ModelID = Settings.Contains("ModelID") ? Settings["ModelID"] : TEXT("");

        UE_LOG(LogTemp, Log, TEXT("  Initializing Model with APIKey=%s, ModelID=%s"), *APIKey, *ModelID);

        TArray<FString> ContextPaths;
        if (Settings.Contains("Context"))
        {
            Settings["Context"].ParseIntoArray(ContextPaths, TEXT(","));
            for (FString& Path : ContextPaths)
            {
                Path = Path.TrimStartAndEnd();
                if (!FPaths::IsRelative(Path))
                {
                    Path = FPaths::ConvertRelativePathToFull(Path);
                }
                else
                {
                    Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + Path);
                }
                UE_LOG(LogTemp, Log, TEXT("    Context File: %s"), *Path);
            }
            Model->ContextFiles = ContextPaths;
        }

        Model->bImportContext = true; // default on
        if (Settings.Contains("ImportContext"))
        {
            Model->bImportContext = Settings["ImportContext"].ToLower() != "false";
        }

        UE_LOG(LogTemp, Log, TEXT("    ImportContext: %s"), Model->bImportContext ? TEXT("true") : TEXT("false"));

        Model->Initialize(APIKey, ModelID);
    };

    if (ModelConfigs.Contains("QueryStepper"))
    {
        ConfigureModel(Stepper, ModelConfigs["QueryStepper"]);
    }
    if (ModelConfigs.Contains("Tagger") && Tagger)
    {
        ConfigureModel(Tagger, ModelConfigs["Tagger"]);
    }
    if (ModelConfigs.Contains("CommandGenerator") && Generator)
    {
        ConfigureModel(Generator, ModelConfigs["CommandGenerator"]);
    }
    if (ModelConfigs.Contains("CommandConsolidator") && Consolidator)
    {
        ConfigureModel(Consolidator, ModelConfigs["CommandConsolidator"]);
    }
    if (ModelConfigs.Contains("CommandVerifier") && Verifier)
    {
        ConfigureModel(Verifier, ModelConfigs["CommandVerifier"]);
    }
}

void UExecutionManager::HandleTerminalOutput(const FString& Output)
{
    // If relay is enabled for the terminal, send its output to the CommandGenerator Model
	if (Terminal->GetRelayEnabled())
	{
		Generator->SendMessage(Output);
	}
}

void UExecutionManager::SendPromptToPipeline(const FString& Prompt, bool bRootAllowed)
{
    UE_LOG(LogTemp, Log, TEXT("🎯 New prompt entered: %s (Root Allowed: %s)"), *Prompt, bRootAllowed ? TEXT("true") : TEXT("false"));

    bRequestedRoot = bRootAllowed;
    LastRespondingModel = Stepper;

    if (Stepper)
    {
        Stepper->SendMessage(Prompt);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("⚠ Cannot send prompt: Stepper model is null."));
    }
}

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

FString ExtractJsonCodeBlock(const FString& RawText)
{
    const FString StartTag = TEXT("```json");
    const FString EndTag = TEXT("```");

    int32 StartIndex = RawText.Find(StartTag);
    if (StartIndex != INDEX_NONE)
    {
        StartIndex += StartTag.Len();
        int32 EndIndex = RawText.Find(EndTag, ESearchCase::IgnoreCase, ESearchDir::FromStart, StartIndex);
        if (EndIndex != INDEX_NONE)
        {
            return RawText.Mid(StartIndex, EndIndex - StartIndex).TrimStartAndEnd();
        }
    }
    return RawText; // fallback to original if no block found
}

void UExecutionManager::OnModelResponse(const FString& Response)
{
    // UE_LOG(LogTemp, Log, TEXT("Model Response Received: %s"), *Response);

    CleanJson = ExtractJsonCodeBlock(Response);

    if (LastRespondingModel == Stepper)
    {
        UE_LOG(LogTemp, Log, TEXT("Routing Stepper → Tagger"));
        LastRespondingModel = Tagger;
        Tagger->SendMessage(CleanJson);
    }
    else if (LastRespondingModel == Tagger)
    {
        UE_LOG(LogTemp, Log, TEXT("Routing Tagger → Generator"));
        LastRespondingModel = Generator;
        Generator->SendMessage(CleanJson);
    }
    else if (LastRespondingModel == Generator)
    {
        UE_LOG(LogTemp, Log, TEXT("Routing Generator → Consolidator"));
        LastRespondingModel = Consolidator;
        Consolidator->SendMessage(CleanJson);
    }
    else if (LastRespondingModel == Consolidator)
    {
        UE_LOG(LogTemp, Log, TEXT("Routing Consolidator → Verifier"));
        LastRespondingModel = Verifier;
        Verifier->SendMessage(CleanJson);
    }
    else if (LastRespondingModel == Verifier)
    {
        UE_LOG(LogTemp, Log, TEXT("Handling Verifier Response"));
    
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(CleanJson);
    
        bool bSuccess = false;
        TArray<FString> CommandsToRun;
    
        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
        {
            FString Status;
            if (JsonObject->TryGetStringField("verification_result", Status) &&
                Status.Equals("success", ESearchCase::IgnoreCase))
            {
                bSuccess = true;
    
                // First try consolidated commands
                const TArray<TSharedPtr<FJsonValue>>* ConsolidatedArray;
                if (JsonObject->TryGetArrayField("consolidated_commands", ConsolidatedArray))
                {
                    for (const TSharedPtr<FJsonValue>& Entry : *ConsolidatedArray)
                    {
                        const TSharedPtr<FJsonObject>* CmdObj;
                        if (Entry->TryGetObject(CmdObj))
                        {
                            FString Combined;
                            if ((*CmdObj)->TryGetStringField("commands", Combined))
                            {
                                CommandsToRun.Add(Combined);
                            }
                        }
                    }
                }
    
                // If no consolidated commands, fallback to individual steps
                if (CommandsToRun.Num() == 0)
                {
                    const TArray<TSharedPtr<FJsonValue>>* StepsArray;
                    if (JsonObject->TryGetArrayField("steps", StepsArray))
                    {
                        for (const TSharedPtr<FJsonValue>& StepVal : *StepsArray)
                        {
                            const TSharedPtr<FJsonObject>* StepObj;
                            if (StepVal->TryGetObject(StepObj))
                            {
                                FString StepCmd;
                                if ((*StepObj)->TryGetStringField("command", StepCmd))
                                {
                                    CommandsToRun.Add(StepCmd);
                                }
                            }
                        }
                    }
                }
            }
        }
    
        if (bSuccess && CommandsToRun.Num() > 0)
        {
            UE_LOG(LogTemp, Log, TEXT("✔ Command Validated. Executing %d command(s):"), CommandsToRun.Num());
            LastRespondingModel = nullptr;
    
            for (const FString& Cmd : CommandsToRun)
            {
                UE_LOG(LogTemp, Log, TEXT("→ Terminal Command: %s"), *Cmd);
                if (Terminal)
                {
                    Terminal->ExecuteCommand(Cmd);
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("❌ Command NOT validated. Looping back to Generator."));
            LastRespondingModel = Generator;
            Generator->SendMessage(CleanJson);
        }
    }

    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Unexpected responder. Starting from Stepper."));
        LastRespondingModel = Stepper;
        Stepper->SendMessage(CleanJson);
    }
}





