#include "CoreMinimal.h"
#include "AICoreLog.h"
#include "HAL/IConsoleManager.h"
#include "String/LexFromString.h"
#include "Engine/World.h"  
#include "rules.h"
#include "rng.h"
#include <algorithm>
#include <vector>
#include <limits>

static void RunAICoreUndoCheck(const TArray<FString>& Args, UWorld* /*World*/)
{
    int32 Steps = 100;
    if (Args.Num() >= 1) LexFromString(Steps, *Args[0]);

    GameState S;
    S.width = 5; S.height = 5; S.sideToAct = 0;
    S.units = { Unit{0,0,12,10,2,true}, Unit{1,1,13,10,2,true} };
    S.initZobrist(0xC0FFEEULL, (int)S.units.size());

    BasicRules R;
    XorShift64Star prng(0xABCDEF1234567890ULL);

    for (int iter = 0; iter < Steps; ++iter) {
        std::vector<Action> moves; R.generateLegal(S, moves);
        if (moves.empty()) break;

        const uint64 prevKey = S.key;
        const Action& a = moves[(size_t)(prng.next() % moves.size())];

        Delta d; R.make(S, a, d); R.unmake(S, d);

        if (S.key != prevKey) {
            UE_LOG(LogAICore, Error, TEXT("[UndoCheck] Mismatch at iter=%d  prev=0x%016llX cur=0x%016llX"),
                iter, (unsigned long long)prevKey, (unsigned long long)S.key);
            return;
        }
    }
    UE_LOG(LogAICore, Log, TEXT("[UndoCheck] OK for %d iterations"), Steps);
}

static FAutoConsoleCommandWithWorldAndArgs CmdAICoreUndoCheck(
    TEXT("AICore.UndoCheck"),
    TEXT("Usage: AICore.UndoCheck <steps=100>  // random make/unmake and verify hash"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RunAICoreUndoCheck)
);