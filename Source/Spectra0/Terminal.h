// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Terminal.generated.h"

class UQueryStepperModel;

UCLASS()
class SPECTRA0_API ATerminal : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATerminal();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Terminal Command Execution
	UFUNCTION(BlueprintCallable, Category = "Terminal")
	void ExecuteCommand(const FString& Command);

	UFUNCTION(BlueprintCallable, Category = "Terminal")
	void ExecuteRootCommand(const TArray<FString>& Commands);

	// Feature Toggles
	UFUNCTION(BlueprintCallable, Category = "Terminal")
	void SetRelayEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Terminal")
	void SetRootAccessEnabled(bool bEnabled);

private:
	bool bUseRoot;
	bool bRelayEnabled;

	// Command History
	TArray<FString> CommandHistory;

public:
	bool GetRelayEnabled() const;
};

