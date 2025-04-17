#include "Terminal.h"
#include "QueryStepperModel.h"

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#endif

// Sets default values
ATerminal::ATerminal()
{
    PrimaryActorTick.bCanEverTick = true;
    bUseRoot = false;
    bRelayEnabled = false;
    
    // Run "pwd"
    {
        FString Cmds = { TEXT("pwd") };
        ExecuteCommand(Cmds);
    }
    // Run "ls"
    {
        FString Cmds = { TEXT("ls") };
        ExecuteCommand(Cmds);
    }
}

// Called when the game starts or when spawned
// void ATerminal::BeginPlay()
// {
//     Super::BeginPlay();
// 
// #if PLATFORM_ANDROID
//     SetRootAccessEnabled(true);
// 
//     // Run "pwd"
//     {
//         TArray<FString> Cmds = { TEXT("pwd") };
//         ExecuteRootCommand(Cmds);
//     }
// 
//     // Run "ls"
//     {
//         TArray<FString> Cmds = { TEXT("ls") };
//         ExecuteRootCommand(Cmds);
//     }
// #endif
// }

void ATerminal::BeginPlay()
{
    Super::BeginPlay();

#if PLATFORM_ANDROID
    // SetRootAccessEnabled(true);
#endif
}

// Called every frame
void ATerminal::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ATerminal::ExecuteCommand(const FString& Command)
{
    if (bUseRoot)
    {
        TArray<FString> Commands;
        Commands.Add(Command);
        ExecuteRootCommand(Commands);
        return;
    }

#if PLATFORM_ANDROID
    // — Android implementation (as before) —
    JNIEnv* Env = FAndroidApplication::GetJavaEnv();
    jstring JCmd = Env->NewStringUTF(TCHAR_TO_UTF8(*Command));

    jclass JavaClass = FAndroidApplication::FindJavaClass("com/epicgames/unreal/GameActivity");
    if (!JavaClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Java class not found: GameActivity"));
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("Java class found: GameActivity"));

    jmethodID ExecMethod = Env->GetStaticMethodID(JavaClass, "exec", "(Ljava/lang/String;)Ljava/lang/String;");
    if (!ExecMethod)
    {
        UE_LOG(LogTemp, Error, TEXT("Static method not found: exec"));
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("Static method found: exec"));

    jstring Result = (jstring)Env->CallStaticObjectMethod(JavaClass, ExecMethod, JCmd);
    const char* UTFChars = Env->GetStringUTFChars(Result, 0);
    FString Output = FString(UTF8_TO_TCHAR(UTFChars));

    Env->ReleaseStringUTFChars(Result, UTFChars);
    Env->DeleteLocalRef(JCmd);
    Env->DeleteLocalRef(Result);

    CommandHistory.Add(Command);
    UE_LOG(LogTemp, Log, TEXT("Android Command Output:\n%s"), *Output);

#elif PLATFORM_WINDOWS
    // — Windows implementation —
    // Create pipes for capturing stdout/stderr
    void* ReadPipe = nullptr;
    void* WritePipe = nullptr;
    FPlatformProcess::CreatePipe(ReadPipe, WritePipe);

    // Launch the process (cmd.exe /C <command>)
    FString CmdExe = TEXT("cmd.exe");
    FString CmdArgs = FString::Printf(TEXT("/C %s"), *Command);
    FProcHandle ProcHandle = FPlatformProcess::CreateProc(
        *CmdExe,
        *CmdArgs,
        /*bLaunchDetached=*/ false,
        /*bLaunchHidden=*/ true,
        /*bLaunchReallyHidden=*/ true,
        /*OutProcessID=*/ nullptr,
        /*Priority=*/ 0,
        /*OptionalWorkingDirectory=*/ nullptr,
        /*PipeWrite=*/ WritePipe,
        /*PipeRead=*/ nullptr
    );

    if (!ProcHandle.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to launch Windows command"));
    }
    else
    {
        // Read until the process exits
        FString Output;
        while (FPlatformProcess::IsProcRunning(ProcHandle))
        {
            FString NewData = FPlatformProcess::ReadPipe(ReadPipe);
            if (!NewData.IsEmpty())
            {
                Output += NewData;
            }
            FPlatformProcess::Sleep(0.01f);
        }
        // Read any remaining data
        Output += FPlatformProcess::ReadPipe(ReadPipe);

        // Close handles
        FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
        FPlatformProcess::CloseProc(ProcHandle);

        CommandHistory.Add(Command);
        UE_LOG(LogTemp, Log, TEXT("Windows Command Output:\n%s"), *Output);
    }
#else
    UE_LOG(LogTemp, Warning, TEXT("ExecuteCommand not supported on this platform"));
#endif
}

void ATerminal::ExecuteRootCommand(const TArray<FString>& Commands)
{
#if PLATFORM_ANDROID
    // — Android root implementation (as before) —
    JNIEnv* Env = FAndroidApplication::GetJavaEnv();
    jclass JavaClass = FAndroidApplication::FindJavaClass("com/epicgames/unreal/GameActivity");
    if (!JavaClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Java class not found: GameActivity"));
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("Java class found: GameActivity"));

    jmethodID SudoMethod = Env->GetStaticMethodID(JavaClass, "sudo", "([Ljava/lang/String;)[Ljava/lang/String;");
    if (!SudoMethod)
    {
        UE_LOG(LogTemp, Error, TEXT("Static method not found: sudo"));
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("Static method found: sudo"));

    jobjectArray JCmdArray = Env->NewObjectArray(Commands.Num(), Env->FindClass("java/lang/String"), nullptr);
    for (int i = 0; i < Commands.Num(); ++i)
    {
        jstring JStr = Env->NewStringUTF(TCHAR_TO_UTF8(*Commands[i]));
        Env->SetObjectArrayElement(JCmdArray, i, JStr);
        Env->DeleteLocalRef(JStr);
    }

    jobjectArray ResultArray = (jobjectArray)Env->CallStaticObjectMethod(JavaClass, SudoMethod, JCmdArray);
    Env->DeleteLocalRef(JCmdArray);

    if (ResultArray != nullptr)
    {
        for (int i = 0; i < 2; ++i)
        {
            jstring JOutStr = (jstring)Env->GetObjectArrayElement(ResultArray, i);
            const char* UTFChars = Env->GetStringUTFChars(JOutStr, 0);
            FString Out = FString(UTF8_TO_TCHAR(UTFChars));

            const FString Tag = (i == 0 ? TEXT("STDOUT") : TEXT("STDERR"));
            UE_LOG(LogTemp, Log, TEXT("Root [%s]:\n%s"), *Tag, *Out);

            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Green, FString::Printf(TEXT("Root [%s]:\n%s"), *Tag, *Out));
            }

            Env->ReleaseStringUTFChars(JOutStr, UTFChars);
            Env->DeleteLocalRef(JOutStr);
        }
        Env->DeleteLocalRef(ResultArray);
    }

    for (const FString& Cmd : Commands)
    {
        CommandHistory.Add(Cmd);
    }

#elif PLATFORM_WINDOWS
    // — Windows root (administrator) implementation —
    // On Windows, elevating a process to admin requires a separate launcher or manifest.
    // Here we'll just execute commands as-is, assuming your game runs elevated.
    for (const FString& Cmd : Commands)
    {
        ExecuteCommand(Cmd);
    }
#else
    UE_LOG(LogTemp, Warning, TEXT("ExecuteRootCommand not supported on this platform"));
#endif
}

void ATerminal::SetRelayEnabled(bool bEnabled)
{
    bRelayEnabled = bEnabled;
    UE_LOG(LogTemp, Log, TEXT("Relay %s"), bEnabled ? TEXT("Enabled") : TEXT("Disabled"));
}

void ATerminal::SetRootAccessEnabled(bool bEnabled)
{
    bUseRoot = bEnabled;
    UE_LOG(LogTemp, Log, TEXT("Root Access %s"), bEnabled ? TEXT("Enabled") : TEXT("Disabled"));
}

bool ATerminal::GetRelayEnabled() const
{
    return bRelayEnabled;
}
