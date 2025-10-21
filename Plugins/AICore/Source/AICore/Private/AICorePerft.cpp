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

static int64 Perft(GameState& S, BasicRules& R, int depth)
{
    if (depth == 0) return 1;
    std::vector<Action> moves;
    R.generateLegal(S, moves);
    int64 nodes = 0;
    for (const auto& a : moves) {
        Delta d; R.make(S, a, d);
        nodes += Perft(S, R, depth - 1);
        R.unmake(S, d);
    }
    return nodes;
}

static void RunAICorePerft(const TArray<FString>& Args, UWorld* /*World*/)
{
    int32 Depth = 2;
    if (Args.Num() >= 1) LexFromString(Depth, *Args[0]);

    GameState S;
    S.width = 5; S.height = 5; S.sideToAct = 0;
    S.units = { Unit{0,0,12,10,2,true}, Unit{1,1,13,10,2,true} };
    S.initZobrist(0xC0FFEEULL, (int)S.units.size());

    BasicRules R;

    // perft
    auto Perft = [&](auto&& self, GameState& GS, int d) -> int64
        {
            if (d == 0) return 1;
            std::vector<Action> moves; R.generateLegal(GS, moves);
            int64 nodes = 0;
            for (const auto& a : moves) { Delta dd; R.make(GS, a, dd); nodes += self(self, GS, d - 1); R.unmake(GS, dd); }
            return nodes;
        };

    int64 nodes = Perft(Perft, S, Depth);
    UE_LOG(LogAICore, Log, TEXT("[Perft] depth=%d nodes=%lld"), Depth, nodes);
}

static FAutoConsoleCommandWithWorldAndArgs CmdAICorePerft(
    TEXT("AICore.Perft"),
    TEXT("Usage: AICore.Perft <depth>  // prints perft nodes"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RunAICorePerft)
);