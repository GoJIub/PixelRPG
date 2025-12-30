#include "../include/druid.h"
#include "../include/npc.h"

Druid::Druid(const std::string &nm, int x_, int y_)
    : NPC(NPCType::Druid, nm, x_, y_)
{
}

InteractionOutcome Druid::accept(IInteractionVisitor &visitor) {
    return visitor.visit(*this);
}
