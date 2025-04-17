#include "AIModel.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

void UAIModel::Initialize(const FString& InApiKey, const FString& InModelID)
{
    ApiKey = InApiKey;
    ModelID = InModelID;
    Endpoint = FString::Printf(
        TEXT("https://generativelanguage.googleapis.com/v1beta/models/%s:streamGenerateContent?key=%s"),
        *ModelID, *ApiKey
    );

    if (bImportContext)
    {
        for (const FString& FilePath : ContextFiles)
        {
            FString FileText;
            if (FFileHelper::LoadFileToString(FileText, *FilePath))
            {
                ConversationHistory.Add({ TEXT("user"), FileText });
                UE_LOG(LogTemp, Log, TEXT("Context loaded from %s"), *FilePath);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to load context from %s"), *FilePath);
            }
        }
    }
}

void UAIModel::SendMessage(const FString& UserInput)
{
    UE_LOG(LogTemp, Log, TEXT("→ Sending to model [%s] (%s): %s"), *GetClass()->GetName(), *ModelID, *UserInput);

    ConversationHistory.Add({ TEXT("user"), UserInput });

    FString Payload = BuildRequestPayload(UserInput);
    DispatchHttpRequest(Payload);
}

void UAIModel::ClearHistory()
{
    ConversationHistory.Empty();
}

void UAIModel::ExportHistoryToFile(const FString& FilePath)
{
    FString ExportText;

    for (const FAIMessage& Message : ConversationHistory)
    {
        ExportText += FString::Printf(TEXT("[%s] %s\n"), *Message.Role, *Message.Content);
    }

    FFileHelper::SaveStringToFile(ExportText, *FilePath);
}

void UAIModel::LoadHistoryFromFile(const FString& FilePath)
{
    FString LoadedText;
    if (FFileHelper::LoadFileToString(LoadedText, *FilePath))
    {
        ConversationHistory.Empty();
        TArray<FString> Lines;
        LoadedText.ParseIntoArrayLines(Lines);
        for (const FString& Line : Lines)
        {
            if (Line.StartsWith("[user] "))
                ConversationHistory.Add({ TEXT("user"), Line.Mid(7) });
            else if (Line.StartsWith("[model] "))
                ConversationHistory.Add({ TEXT("model"), Line.Mid(8) });
        }
    }
}

FString UAIModel::ExtractCleanResponseText(const FString& JsonString) const
{
    TSharedPtr<FJsonValue> RootValue;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, RootValue) || !RootValue.IsValid())
    {
        return TEXT("Error: Invalid JSON response.");
    }

    const TArray<TSharedPtr<FJsonValue>>* RootArray = nullptr;
    if (!RootValue->TryGetArray(RootArray) || RootArray->Num() == 0)
    {
        return TEXT("Error: Expected JSON array in response.");
    }

    FString CollectedText;

    // Loop through each top‑level element
    for (const TSharedPtr<FJsonValue>& ElementValue : *RootArray)
    {
        const TSharedPtr<FJsonObject>* ElementObj = nullptr;
        if (!ElementValue->TryGetObject(ElementObj))
        {
            continue;
        }

        // Get "candidates" array
        const TArray<TSharedPtr<FJsonValue>>* Candidates = nullptr;
        if (!(*ElementObj)->TryGetArrayField(TEXT("candidates"), Candidates))
        {
            continue;
        }

        // Loop through each candidate
        for (const TSharedPtr<FJsonValue>& CandValue : *Candidates)
        {
            const TSharedPtr<FJsonObject>* CandObj = nullptr;
            if (!CandValue->TryGetObject(CandObj))
            {
                continue;
            }

            // Get "content" object
            const TSharedPtr<FJsonObject>* ContentObj = nullptr;
            if (!(*CandObj)->TryGetObjectField(TEXT("content"), ContentObj))
            {
                continue;
            }

            // Get "parts" array
            const TArray<TSharedPtr<FJsonValue>>* Parts = nullptr;
            if (!(*ContentObj)->TryGetArrayField(TEXT("parts"), Parts))
            {
                continue;
            }

            // Loop through each part and append its "text"
            for (const TSharedPtr<FJsonValue>& PartValue : *Parts)
            {
                const TSharedPtr<FJsonObject>* PartObj = nullptr;
                if (!PartValue->TryGetObject(PartObj))
                {
                    continue;
                }

                FString PartText;
                if ((*PartObj)->TryGetStringField(TEXT("text"), PartText))
                {
                    CollectedText += PartText;
                }
            }
        }
    }

    return CollectedText.IsEmpty()
        ? TEXT("No text found in model response.")
        : CollectedText;
}

FString UAIModel::BuildRequestPayload(const FString& InputText) const
{
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Contents;

    // Add prior history
    for (const FAIMessage& Message : ConversationHistory)
    {
        TSharedRef<FJsonObject> ContentObject = MakeShared<FJsonObject>();
        ContentObject->SetStringField("role", Message.Role);

        TArray<TSharedPtr<FJsonValue>> Parts;
        TSharedRef<FJsonObject> Part = MakeShared<FJsonObject>();
        Part->SetStringField("text", Message.Content);
        Parts.Add(MakeShared<FJsonValueObject>(Part));

        ContentObject->SetArrayField("parts", Parts);
        Contents.Add(MakeShared<FJsonValueObject>(ContentObject));
    }

    // Add current input
    TSharedRef<FJsonObject> UserInputObject = MakeShared<FJsonObject>();
    UserInputObject->SetStringField("role", "user");

    TArray<TSharedPtr<FJsonValue>> UserParts;
    TSharedRef<FJsonObject> UserPart = MakeShared<FJsonObject>();
    UserPart->SetStringField("text", InputText);
    UserParts.Add(MakeShared<FJsonValueObject>(UserPart));

    UserInputObject->SetArrayField("parts", UserParts);
    Contents.Add(MakeShared<FJsonValueObject>(UserInputObject));

    Root->SetArrayField("contents", Contents);

    TSharedRef<FJsonObject> Config = MakeShared<FJsonObject>();
    Config->SetStringField("responseMimeType", "text/plain");
    Root->SetObjectField("generationConfig", Config);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(Root, Writer);

    return OutputString;
}

void UAIModel::DispatchHttpRequest(const FString& JsonPayload)
{
    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Endpoint);
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(JsonPayload);
    Request->OnProcessRequestComplete().BindUObject(this, &UAIModel::HandleHttpResponse);
    Request->ProcessRequest();
}

void UAIModel::HandleHttpResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid())
    {
        // Get full JSON response
        FString FullJson = Response->GetContentAsString();

        // Extract clean text from JSON
        FString CleanText = ExtractCleanResponseText(FullJson);

        // Add clean text to history
        ConversationHistory.Add({ TEXT("model"), CleanText });

        UE_LOG(LogTemp, Log, TEXT("← Response from model [%s] (%s): %s"), *GetClass()->GetName(), *ModelID, *CleanText);

        // Broadcast clean response
        OnResponseReceived.Broadcast(CleanText);
    }
    else
    {
        FString Error = Response.IsValid() ? Response->GetContentAsString() : TEXT("No response");
        UE_LOG(LogTemp, Error, TEXT("HTTP Request failed: %s"), *Error);
    }
}


