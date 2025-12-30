#include "../include/orc.h"
#include "../include/npc.h"

Orc::Orc(const std::string &nm, int x_, int y_)
    : NPC(NPCType::Orc, nm, x_, y_)
{
}

InteractionOutcome Orc::accept(IInteractionVisitor &visitor) {
    return visitor.visit(*this);
}
