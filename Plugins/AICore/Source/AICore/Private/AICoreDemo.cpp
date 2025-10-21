#include "CoreMinimal.h"
#include "AICoreLog.h"
#include "HAL/IConsoleManager.h"
#include "rules.h"

static void RunAICoreDemo()
{
    GameState S;
    S.width = 5; S.height = 5; S.sideToAct = 0;
    S.units = { Unit{0,0,12,10,2,true}, Unit{1,1,13,10,2,true} };
    S.initZobrist(0xC0FFEEULL, (int)S.units.size());

    BasicRules R;
    std::vector<Action> actions;
    R.generateLegal(S, actions);

    UE_LOG(LogAICore, Log, TEXT("[Demo] legal_actions=%d"), (int)actions.size());

    if (!actions.empty()) {
        Delta d; R.make(S, actions[0], d);
        UE_LOG(LogAICore, Log, TEXT("[Demo] after first action key=0x%016llX"), (unsigned long long)S.key);
        R.unmake(S, d);
        UE_LOG(LogAICore, Log, TEXT("[Demo] after unmake key=0x%016llX"), (unsigned long long)S.key);
    }
}

static FAutoConsoleCommand CmdAICoreDemo(
    TEXT("AICore.Demo"),
    TEXT("Run AICore demo (prints legal actions and state hash)"),
    FConsoleCommandDelegate::CreateStatic(&RunAICoreDemo)
);