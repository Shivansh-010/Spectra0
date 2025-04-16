// AIModel.h
#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "UObject/NoExportTypes.h"
#include "AIModel.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIResponse, const FString&, Response);

USTRUCT(BlueprintType)
struct FAIMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString Role; // "user" or "model"

	UPROPERTY(BlueprintReadWrite)
	FString Content;
};

UCLASS(Blueprintable)
class SPECTRA0_API UAIModel : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "AI Model")
	virtual void Initialize(const FString& InApiKey, const FString& InModelID);

	UFUNCTION(BlueprintCallable, Category = "AI Model")
	virtual void SendMessage(const FString& UserInput);

	UFUNCTION(BlueprintCallable, Category = "AI Model")
	virtual void ClearHistory();

	UFUNCTION(BlueprintCallable, Category = "AI Model")
	virtual void ExportHistoryToFile(const FString& FilePath);

	UFUNCTION(BlueprintCallable, Category = "AI Model")
	virtual void LoadHistoryFromFile(const FString& FilePath);

	// Context file configuration
	UPROPERTY(BlueprintReadWrite, Category = "Context")
	TArray<FString> ContextFiles;

	UPROPERTY(BlueprintReadWrite, Category = "Context")
	bool bImportContext = true;

	UPROPERTY(BlueprintAssignable, Category = "AI Model")
	FOnAIResponse OnResponseReceived;

protected:
	FString ApiKey;
	FString ModelID;
	FString Endpoint;

	TArray<FAIMessage> ConversationHistory;
	
	FString ExtractCleanResponseText(
		const FString& JsonString) const;
	virtual FString BuildRequestPayload(const FString& InputText) const;
	virtual void DispatchHttpRequest(const FString& JsonPayload);
	virtual void HandleHttpResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
