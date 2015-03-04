#include "stdafx.h"
#include "precompiled_objects.h"

#include "aircraft_model.h"
#include "aircraft_common.h"

#include "sync_fsm/sync_phys_state.h"
#include "sync_fsm/sync_none_state.h"
#include "sync_fsm/sync_fms_state.h"

namespace aircraft
{

// FIXME ���� ����� ����
void /*system_base::*/block_obj_msgs(bool block)
{ }

void /*system_base::*/send_obj_message(size_t object_id, binary::bytes_cref bytes, bool sure, bool just_cmd)
{}

object_info_ptr create(kernel::system_ptr sys,nodes_management::manager_ptr nodes_manager,phys::control_ptr        phys)
{
    size_t id  = 0x666;
    std::vector<object_info_ptr>  objects;
    objects.push_back(nodes_manager);
    auto msg_service = boost::bind(&send_obj_message, id, _1, _2, _3);
    auto block_msgs  = [=](bool block){ block_obj_msgs(block); };
    kernel::object_create_t  oc(
        nullptr, 
        sys.get(),                  // kernel::system*                 sys             , 
        id,                         // size_t                          object_id       , 
        "name",                     // string const&                   name            , 
        objects,  // vector<object_info_ptr> const&  objects         , 
        msg_service,                // kernel::send_msg_f const&       send_msg        , 
        block_msgs                  // kernel::block_obj_msgs_f        block_msgs
        );

    aircraft::settings_t air_settings;
    dict_t d = dict::wrap(craft_data(air_settings));

    return model::create(phys,oc,d);
}


object_info_ptr model::create(phys::control_ptr        phys,kernel::object_create_t const& oc, dict_copt dict)
{
    return object_info_ptr(new model(phys, oc, dict));
}

AUTO_REG_NAME(aircraft_model, model::create);

model::model( phys::control_ptr        phys, kernel::object_create_t const& oc, dict_copt dict )
    : view(oc,dict)
    , desired_velocity_(min_desired_velocity())
    , phys_            (phys)
    , airports_manager_(find_first_object<airports_manager::info_ptr>(collection_))
    , fast_session_    (false)
    , nm_ang_smooth_   (2)
{

    if (get_nodes_manager())
    {
        root_               = get_nodes_manager()->get_node(0);
        elev_rudder_node_   = get_nodes_manager()->find_node("elevrudr");
        rudder_node_        = get_nodes_manager()->find_node("rudder");
        tow_point_node_     = get_nodes_manager()->find_node("tow_point");
        body_node_          = get_nodes_manager()->find_node("body");
    }

    shassis_ = boost::make_shared<shassis_support_impl>(get_nodes_manager());
    sync_state_.reset(new sync_fsm::none_state(*this));

     FIXME(����� ���������� �������)
    //conn_holder() << dynamic_cast<system_session *>(sys_)->subscribe_time_factor_changed(boost::bind(&model::on_time_factor_changed, this, _1, _2));

}

void model::update( double time )
{   
    view::update(time);
    update_len(time);

    //if (!fpl_ || fpl_->active())
    //    aircraft_fms::model_control_ptr(get_fms_info())->activate();
        
    FIXME(init_shassi_anim)
    //init_shassi_anim();

    double dt = time - (last_update_ ? *last_update_ : 0);
    if (cg::eq_zero(dt))
        return;

    desired_nm_pos_.reset();
    desired_nm_orien_.reset();

    sync_fsm::state_ptr prev_state = sync_state_;
    sync_state_->update(time, dt);

    FIXME(������������� ����)

    //sync_nm_root(dt);

    update_contact_effects(time);
    check_wheel_brake();
    FIXME(update_shassi_anim)
    //update_shassi_anim(time);
    update_atc_state();
    sync_fms();

    if (phys_aircraft_)
    {
        bool has_malfunction = false;
        if (malfunction(MF_CHASSIS_FRONT) || malfunction(MF_CHASSIS_REAR_LEFT) || malfunction(MF_CHASSIS_REAR_RIGHT))
            has_malfunction = true;

        phys_aircraft_->set_malfunction(has_malfunction);
    }

    last_update_ = time;
}

#if 0

void model::update(double dt)
{
    auto it = this;

    if((*it).traj_.get())
    {
        if ((*it).traj_->cur_len() < (*it).traj_->length())
        {
            (*it).phys_aircraft_->set_prediction(15.); 
            (*it).phys_aircraft_->freeze(false);
            const double  cur_len = (*it).traj_->cur_len();
            (*it).traj_->set_cur_len ((*it).traj_->cur_len() + dt*(*it).desired_velocity_);
            const double  tar_len = (*it).traj_->cur_len();
            decart_position target_pos;

            target_pos.pos = cg::point_3((*it).traj_->kp_value(tar_len),0);
            geo_position gtp(target_pos, get_base());
            (*it).phys_aircraft_->go_to_pos(gtp.pos ,gtp.orien);


            const double curs_change = (*it).traj_->curs_value(tar_len) - (*it).traj_->curs_value(cur_len);

            if(cg::eq(curs_change,0.0))
                (*it).desired_velocity_ = aircraft::max_desired_velocity();
            else
                (*it).desired_velocity_ = aircraft::min_desired_velocity();

            // const decart_position cur_pos = _phys_aircrafts[0].phys_aircraft_->get_local_position();

            //std::stringstream cstr;

            //cstr << std::setprecision(8) 
            //     << "curr_pods_len:  "             << (*it).traj->cur_len() 
            //     << "    desired_velocity :  "     << (*it).desired_velocity_   
            //     << "    delta curs :  "  << curs_change
            //     << ";   cur_pos x= "     << cur_pos.pos.x << " y= "  << cur_pos.pos.y  
            //     << "    target_pos x= "  << target_pos.pos.x << " y= "  << target_pos.pos.y <<"\n" ;

            //OutputDebugString(cstr.str().c_str());
        }
        else
        {

            cg::point_3 cur_pos = phys_aircraft_->get_local_position().pos;
            cg::point_3 d_pos = phys_aircraft_->get_local_position().dpos;
            cg::point_3 trg_p((*it).traj_->kp_value((*it).traj_->length()),0);
            d_pos.z = 0;
            if(cg::distance(trg_p,cur_pos) > 1.0 && cg::norm(d_pos) > 0.05)
            {   
                decart_position target_pos;
                target_pos.pos = trg_p;
                geo_position gp(target_pos, get_base());
                (*it).phys_aircraft_->go_to_pos(gp.pos ,gp.orien);
            }
            else
            {
                // (*it).traj.reset();
                (*it).phys_aircraft_->freeze(true);
            }
        }

    }

    if(phys_aircraft_)
        phys_aircraft_->update();

}

#endif

airports_manager::info_ptr model::get_airports_manager() const
{
    return airports_manager_;
}

phys::control_ptr model::phys_control() const
{
    return phys_;
}

nodes_management::manager_ptr model::get_nodes_manager() const
{
    return view::get_nodes_manager();
}

aircraft_fms::info_ptr model::get_fms_info() const
{
    return view::get_fms_info();
}

shassis_support_ptr model::get_shassis() const
{
    return shassis_;
}

geo_position model::get_root_pos() const
{
    FIXME("������� ���� � ���������� ����������� ��������� ������� ������")
    return geo_position();//root_->position().global();
}

bool model::is_fast_session() const
{
    return fast_session_;
}

void model::set_desired_nm_pos  (geo_point_3 const& pos)
{
    desired_nm_pos_ = pos;
}

void model::set_desired_nm_orien(quaternion const& orien)
{
    desired_nm_orien_ = orien;
}

// from view
#pragma region view

nodes_management::node_info_ptr model::root() const
{
    //return nodes_manager_->get_node(0);  // FIXME ��������� �� �������� ������
    return get_nodes_manager()->find_node("root");
}


#pragma  endregion

void model::on_malfunction_changed( malfunction_kind_t kind ) 
{
    if (kind == MF_CHASSIS_FRONT)
    {
        shassis_->set_malfunction(SG_FRONT, malfunction(kind));
    }
    else if (kind == MF_CHASSIS_REAR_LEFT)
    {
        shassis_->set_malfunction(SG_REAR_LEFT, malfunction(kind));
    }
    else if (kind == MF_CHASSIS_REAR_RIGHT)
    {
        shassis_->set_malfunction(SG_REAR_RIGHT, malfunction(kind));
    }
    else if (kind == MF_ONE_ENGINE || kind == MF_ALL_ENGINES)
    {
        double factor = 1;
        if (malfunction(MF_ALL_ENGINES))
            factor = 0;
        else if (malfunction(MF_ONE_ENGINE))
            factor = 0.7;

        // FIXME TODO OR NOT TODO
        //auto controls = flight_manager_control_->get_controls();
        //controls.engine_factor = factor;
        //flight_manager_control_->set_controls(controls);
    }
    else if (kind == MF_FUEL_LEAKING)
    {   
        // FIXME TODO OR NOT TODO
        //auto controls = flight_manager_control_->get_controls();
        //controls.fuel_leaking = true;
        //flight_manager_control_->set_controls(controls);
    }
}

void model::update_contact_effects(double time)
{
    if (!phys_aircraft_)
        return;

    auto contacts = std::move(phys_aircraft_->get_body_contacts());
    if (!contacts.empty())
    {
        vector<msg::contact_effect::contact_t> contacts_to_send;

        for (auto it = contacts.begin(); it != contacts.end(); ++it)
            contacts_to_send.push_back(msg::contact_effect::contact_t(it->offset, it->vel));

        set(msg::contact_effect(move(contacts_to_send), time), false);
    }

    shassis_->visit_chassis([this, time](shassis_group_t const&, shassis_t& shassis)
    {
        bool has_contact = false;
        for (size_t  i = 0 ; i < shassis.phys_wheels.size(); ++i)
            if (this->phys_aircraft_->has_wheel_contact(shassis.phys_wheels[i]))
                has_contact = true;

        if (has_contact)
        {
            //             double skid = this->phys_aircraft_->wheel_skid_info(shassis.phys_wheels[0]);
            if (!shassis.landing_dust)
            {
                geo_position wpos = this->phys_aircraft_->get_wheel_position(shassis.phys_wheels[0]);
                geo_position body_pos = this->phys_aircraft_->get_position();
                cg::point_3 loc_omega = (!body_pos.orien).rotate_vector(wpos.omega);
                cg::point_3 vel = body_pos.dpos - body_pos.orien.rotate_vector(cg::point_3(loc_omega.x,0,0) * cg::grad2rad()) * shassis.radius;

                if (cg::norm(vel) > 50)
                {
                    point_3 loc_offset = (!body_pos.orien).rotate_vector(body_pos.pos(wpos.pos));
                    loc_offset += cg::point_3(0,0,shassis.radius);
                    this->set(msg::wheel_contact_effect(time, loc_offset, vel), false);

                    shassis.landing_dust = true;
                }
            }
        }
        else
        {
            shassis.landing_dust = false;
        }

    });
}



void model::sync_fms(bool force)
{
	if (!phys_aircraft_)
		return ;
#if 0
	geo_position fmspos = fms_pos();
	geo_position physpos = phys_aircraft_->get_position();

	double prediction = 300;

	if (force || tow_attached_ || (cg::distance2d(fmspos.pos, physpos.pos) > cg::norm(physpos.dpos) * prediction * sys_->calc_step() * 2))
		aircraft_fms::model_control_ptr(get_fms_info())->reset_pos(physpos.pos, physpos.orien.cpr().course);
#endif
}

phys::rigid_body_ptr model::get_rigid_body() const
{
    return phys_aircraft_ ? phys_aircraft_->get_rigid_body() : phys::rigid_body_ptr();
}

point_3 model::tow_offset() const
{
    return tow_point_node_ ? get_relative_transform(get_nodes_manager(), tow_point_node_, body_node_).translation() : point_3();
}

bool model::tow_attached() const
{
    return tow_attached_;
}
geo_position model::get_phys_pos() const
{
    // TODO
    return phys_aircraft_->get_position();
}

void model::set_tow_attached(optional<uint32_t> attached, boost::function<void()> tow_invalid_callback)
{
    if (tow_attached_ == attached)
        return ;

    tow_attached_ = attached;
    tow_invalid_callback_ = tow_invalid_callback;
    if (phys_aircraft_)
        phys_aircraft_->attach_tow(attached);

    if (!tow_attached_)
        sync_fms(true);
}

void model::set_steer( double steer )
{   
    Assert(tow_attached_);

    if (phys_aircraft_)
        phys_aircraft_->set_steer(steer);
}

geo_position model::fms_pos() const
{
    // FIXME  
    //point_3 dir = cg::polar_point_3(1., get_fms_info()->get_state().orien().course, get_fms_info()->get_state().orien().pitch);
    //return geo_position(get_fms_info()->get_state().dyn_state.pos, get_fms_info()->get_state().dyn_state.TAS * dir, get_fms_info()->get_state().orien(), point_3());
    return geo_position();
}

void model::switch_sync_state(sync_fsm::state_ptr state)
{
    if (sync_state_)
        sync_state_->deinit();
    sync_state_ = state;
}

void model::freeze_position()
{
    auto  fmspos = fms_pos();
    nodes_management::node_position root_node_pos(geo_position(fmspos.pos, fmspos.orien));
    root_->set_position(root_node_pos);
}

void model::set_phys_aircraft(phys_aircraft_ptr phys_aircraft)
{
    if (!phys_aircraft)
    {
        if (tow_attached_ && tow_invalid_callback_)
            tow_invalid_callback_();
    }
    phys_aircraft_ = phys_aircraft;
}

void model::set_nm_angular_smooth(double val)
{
    nm_ang_smooth_ = val;
}



void model::check_wheel_brake()
{
    if (!phys_aircraft_)
        return;

    shassis_->visit_groups([this](shassis_group_t & shassis_group)
    {
        if (shassis_group.opened && shassis_group.malfunction && !shassis_group.broken)
        {
            bool has_contact = shassis_group.check_contact(this->phys_aircraft_);
            if (has_contact)
                shassis_group.broke(this->phys_aircraft_);
        }
    });
}

void model::on_time_factor_changed(double /*time*/, double factor)
{
    //bool fast = factor > 1 ? true : false;

    //if (fast_session_ != fast)
    //{

    //    if (sync_state_)
    //    {
    //        sync_fsm::state_ptr prev_state = sync_state_;
    //        sync_state_->on_fast_session(fast);
    //    }
    //    fast_session_ = fast;
    //}
}


}
