#include "../include/dragon.h"
#include "../include/npc.h"

Dragon::Dragon(const std::string &nm, int x_, int y_)
    : NPC(NPCType::Dragon, nm, x_, y_)
{
}

InteractionOutcome Dragon::accept(IInteractionVisitor &visitor) {
    return visitor.visit(*this);
}
