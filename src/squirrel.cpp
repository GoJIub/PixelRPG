#include "../include/squirrel.h"
#include "../include/npc.h"

Squirrel::Squirrel(const std::string &nm, int x_, int y_)
    : NPC(NPCType::Squirrel, nm, x_, y_)
{
}

InteractionOutcome Squirrel::accept(IInteractionVisitor &visitor) {
    return visitor.visit(*this);
}
