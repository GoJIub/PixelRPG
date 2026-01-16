#pragma once
#include "npc.h"

struct Dragon : public NPC {
    Dragon() = default;
    Dragon(const std::string &nm, int x_, int y_);
    InteractionOutcome accept(IInteractionVisitor &visitor) override;
};
