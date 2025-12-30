#include "../include/bear.h"
#include "../include/npc.h"

Bear::Bear(const std::string &nm, int x_, int y_)
    : NPC(NPCType::Bear, nm, x_, y_)
{
}

InteractionOutcome Bear::accept(IInteractionVisitor &visitor) {
    return visitor.visit(*this);
}
