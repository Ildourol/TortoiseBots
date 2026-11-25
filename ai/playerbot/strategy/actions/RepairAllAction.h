#pragma once
#include "playerbot/PlayerbotAI.h"
#include "GenericActions.h"

namespace ai
{
    class RepairAllAction : public ChatCommandAction
    {
    public:
        RepairAllAction(PlayerbotAI* ai) : ChatCommandAction(ai, "repair") {}
        virtual bool Execute(Event& event) override;

        // What a repair order that repaired nothing says why: `.bot action
        // repair`, the TBM Repair button.
        static std::string RefusalReason(PlayerbotAI* botAI);
    };
}
