#pragma once

#include "Curve.h"
#include "Session.h"
#include <string>

namespace xp_control {
struct Snapshot {
    Curve configured{};
    Curve engine{};
    bool initialized{};
    bool busy{};
    bool canApply{};
    bool hasPlayer{};
    bool engineValid{};
    bool experience{};
    unsigned level{};
    float xp{};
    float threshold{};
    std::string status{"Waiting for game data."};
};

void InitializeRuntime();
void ResetSession(Phase phase);
void StartSession();
Snapshot GetSnapshot();
void RequestSnapshot();
void RequestApply(Curve curve);
bool RegisterMenu();
}
