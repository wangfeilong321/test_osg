#pragma once

#include "sync_pl_fsm.h"

namespace aircraft_physless
{
namespace sync_fsm
{
    struct none_state : state_t
    {
        none_state(self_t &self)
            : self_(self)
        {
            auto fms_pos = self_.fms_pos();
            self_.freeze_position();
        }

        void update(double /*time*/, double dt);
 
    private:
        self_t &self_;

    public:
        static const double sync_none_dist;
    };
}
}