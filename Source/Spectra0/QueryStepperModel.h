
#pragma once

#include "AIModel.h"
#include "QueryStepperModel.generated.h"

UCLASS()
class SPECTRA0_API UQueryStepperModel : public UAIModel
{
	GENERATED_BODY()

public:
	virtual void Initialize(const FString& ApiKey = TEXT("AIzaSyBLMCn2O7CtAbGk1wSjXM3pl4WfVr0Td_E"), const FString& ModelIDOverride = TEXT("")) override;
};

