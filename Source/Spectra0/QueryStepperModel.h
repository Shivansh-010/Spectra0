
#pragma once

#include "AIModel.h"
#include "QueryStepperModel.generated.h"

UCLASS()
class SPECTRA0_API UQueryStepperModel : public UAIModel
{
	GENERATED_BODY()

public:
	virtual void Initialize(const FString& ApiKey = TEXT(""), const FString& ModelIDOverride = TEXT("")) override;
};

