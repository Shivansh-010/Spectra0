// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ExecutionManager.h"
#include "Spectra0GameMode.generated.h"

/**
 * 
 */
UCLASS()
class SPECTRA0_API ASpectra0GameMode : public AGameModeBase
{
	GENERATED_BODY()
	
	virtual void BeginPlay() override;

public:	
	UPROPERTY(BlueprintReadWrite)
	UExecutionManager* ExecutionManager;

};
