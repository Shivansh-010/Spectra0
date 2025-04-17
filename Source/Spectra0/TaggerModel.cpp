// Fill out your copyright notice in the Description page of Project Settings.


#include "TaggerModel.h"

#include "Misc/Paths.h"

void UTaggerModel::Initialize(const FString& InApiKey, const FString& ModelIDOverride)
{
	FString UseModelID = ModelIDOverride.IsEmpty() ? TEXT("gemini-2.0-flash") : ModelIDOverride;

	/*ContextFiles = {
		FPaths::ProjectContentDir() + TEXT("Context/Tagger.md")
	};*/
	bImportContext = true;

	Super::Initialize(InApiKey, UseModelID);
}
