// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIModel.h"
#include "CommandConsolidatorModel.generated.h"

UCLASS()
class SPECTRA0_API UCommandConsolidatorModel : public UAIModel
{
	GENERATED_BODY()

public:
	virtual void Initialize(const FString& InApiKey, const FString& ModelIDOverride = TEXT("")) override;
};

