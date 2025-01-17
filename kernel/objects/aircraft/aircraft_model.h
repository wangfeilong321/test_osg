#pragma once

#include "aircraft_view.h"
#include "phys/phys_sys.h"
#include "common/phys_object_model_base.h"
#include "phys/aircraft.h"
#include "sync_fsm/sync_fsm.h"

#include "network/msg_dispatcher.h"

using network::gen_msg;  // FIXME
using network::msg_t;

#include "aircraft_msg.h"

namespace aircraft
{
    struct model
        : view
        , model_info                // ��������� ���������� � ������
        , model_control             // ��������� ���������� �������
        , int_control
        , sync_fsm::self_t
        , phys_object_model_base 
    {
        static object_info_ptr create(kernel::object_create_t const& oc, dict_copt dict);

    private:

        model( kernel::object_create_t const& oc, dict_copt dict);

    public:
        void on_malfunction_changed( malfunction_kind_t kind ) override; 
        void on_new_contact_effect      (double /*time*/, std::vector<contact_t> const& /*contacts*/) override;

    // base_presentation
        FIXME(private)
        void update(double dt);

        // base_view_presentation
    protected:
        void on_child_removing(kernel::object_info_ptr child) override ;
        void on_object_destroying(object_info_ptr object)     override ;

        // model_info
    private:
        phys::rigid_body_ptr get_rigid_body() const;
        point_3              tow_offset    () const;
        bool                 tow_attached  () const;
        geo_position         get_phys_pos  () const;
        double               rotors_angular_speed () const override;

        // model_control
    private:
        void                 set_tow_attached(optional<uint32_t> attached, boost::function<void()> tow_invalid_callback);
        void                 set_steer                ( double steer ) override;
        void                 set_brake                ( double brake ) override;
        void                 set_rotors_angular_speed ( double val ) override;

        // sync_fsm::self_t
    private:
        geo_position           fms_pos() const;
        airports_manager::info_ptr get_airports_manager() const;
        phys::control_ptr      phys_control() const;
        nodes_management::manager_ptr get_nodes_manager() const;
        aircraft_fms::info_ptr get_fms_info() const;
        //meteo::meteo_cursor_ptr get_meteo_cursor() const;
        shassis_support_ptr    get_shassis() const;
        rotors_support_ptr     get_rotors() const;

        fms::trajectory_ptr    get_trajectory() const;

        geo_position           get_root_pos() const;
        bool is_fast_session() const;
        void set_desired_nm_pos  (geo_point_3 const& pos);
        void set_desired_nm_orien(quaternion const& orien);
        void switch_sync_state(sync_fsm::state_ptr state);
        void freeze_position();
        void set_phys_aircraft(phys_aircraft_ptr phys_aircraft);
        void set_nm_angular_smooth(double val);
    private:
        void on_zone_destroyed( size_t id );
        void update_model();
        void sync_phys(double dt);
        void calc_phys_controls(double & slide_angle, double & thrust, double & attack_angle, double q, cg::rotation_3 const& vel_rotation, point_3 const& desired_accel, point_3 const& wind, bool reverse, bool low_attack);
        void sync_fms(bool force = false);
        bool create_phys_aircraft_impl();  // � ��� ������ �����
        void init_shassi_anim ();
        void update_shassi_anim (double time);
        void update_contact_effects(double time);
        void check_wheel_brake();
        void check_rotors_malfunction();
        void on_time_factor_changed(double time, double factor);
            
        void sync_nm_root(double dt);

   public:
        inline aircraft::shassis_support_ptr get_chassis() {return shassis_;};
        decart_position get_local_position() {return phys_aircraft_->get_local_position();};
        
        void set_trajectory(fms::trajectory_ptr  traj){traj_ = traj;}
        fms::trajectory_ptr  get_trajectory(){return traj_;} 

    private:
        model_system *                         sys_;
        optional<double>                       last_update_;

        airports_manager::info_ptr             airports_manager_;
        
        nodes_management::node_control_ptr     root_;
        nodes_management::node_control_ptr     elev_rudder_node_;
        nodes_management::node_control_ptr     rudder_node_;
        nodes_management::node_info_ptr        tow_point_node_;
        nodes_management::node_info_ptr        body_node_;

        rotors_support_ptr                     rotors_;
        shassis_support_ptr                    shassis_;
        optional<double>                       last_shassi_play_;
        bool                                   shassi_anim_inited_;

        double                                 rotors_angular_speed_;

        aircraft::phys_aircraft_ptr            phys_aircraft_;

        optional<geo_point_3>                  desired_nm_pos_;
        optional<quaternion>                   desired_nm_orien_;
        double                                 nm_ang_smooth_;
        
        boost::function<void()>                tow_invalid_callback_;
        optional<uint32_t>                     tow_attached_;

    private:
        bool fast_session_;

    private:
        sync_fsm::state_ptr sync_state_;

    FIXME(��������� ����������� ����������)
public:
    static const double shassi_height_;

    };


}