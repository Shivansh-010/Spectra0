
#include "QueryStepperModel.h"
#include "Misc/Paths.h"

void UQueryStepperModel::Initialize(const FString& InApiKey, const FString& ModelIDOverride)
{
    FString UseModelID = ModelIDOverride.IsEmpty() ? TEXT("gemini-2.0-flash") : ModelIDOverride;

    ContextFiles = {
        FPaths::ProjectContentDir() + TEXT("Context/QueryStepper.md")
    };
    bImportContext = true;

    Super::Initialize(InApiKey, UseModelID);
}
