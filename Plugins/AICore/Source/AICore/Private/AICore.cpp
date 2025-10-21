// Copyright Epic Games, Inc. All Rights Reserved.
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "AICoreLog.h"

DEFINE_LOG_CATEGORY(LogAICore);

class FAICoreModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogAICore, Log, TEXT("AICore module started."));
    }
    virtual void ShutdownModule() override
    {
        UE_LOG(LogAICore, Log, TEXT("AICore module shutdown."));
    }
};

IMPLEMENT_MODULE(FAICoreModule, AICore)