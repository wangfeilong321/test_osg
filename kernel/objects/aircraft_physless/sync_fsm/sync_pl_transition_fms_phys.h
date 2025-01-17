#pragma once

#include "sync_pl_fsm.h"
#include "../phys_aircraft_phl.h"

namespace aircraft_physless
{
namespace sync_fsm
{
    struct transition_fms_phys_state : state_t
    {
        transition_fms_phys_state(self_t &self, phys_aircraft_ptr phys_aircraft, geo_base_3 const& base, double time)
            : self_(self)
            , phys_aircraft_(phys_aircraft)
            , base_(base)
            , start_transition_time_(time)
        {
        }

        void update(double time, double dt);
        void on_zone_destroyed( size_t id );

    private:
    private:
        self_t &self_;
        double start_transition_time_;
        geo_base_3 base_;
        size_t zone_;

        phys_aircraft_ptr phys_aircraft_;
    };

}
}