#pragma once

#include "geometry/curve.h"

namespace fms
{

inline double to_dubin(const cg::quaternion& orien)
{
	return cg::grad2rad()*(90 - orien.get_course());
}

inline cg::quaternion from_dubin(double course)
{
	return cg::cpr(90 - cg::rad2grad()*course);
}

struct trajectory;

typedef polymorph_ptr<trajectory> trajectory_ptr;

struct traj_data
{ 
    traj_data()
        : speed_seg_(sd_segments_t())            
    {}

    typedef cg::curve_t<cg::point_3>         keypoints_t;
    typedef cg::curve_t<cg::quaternion>      curses_t;
    typedef cg::curve_t<double>              speed_t;

    typedef std::vector<keypoints_t>         kp_segments_t;
    typedef std::vector<curses_t>            cr_segments_t; 
    typedef std::vector<speed_t>             sd_segments_t; 

    kp_segments_t                     kp_seg_;
    cr_segments_t                     curs_seg_; 
    boost::optional<sd_segments_t>    speed_seg_; 

    REFL_INNER(traj_data)
        REFL_ENTRY( kp_seg_   )
        REFL_ENTRY( curs_seg_ )
        REFL_ENTRY( speed_seg_  )
    REFL_END()
};




struct trajectory : traj_data
{
    static trajectory_ptr create(const decart_position& begin_pos,decart_position const& end_pos,double radius, double step = 1); 
    static trajectory_ptr create(const traj_data& data); 
    static trajectory_ptr create(const keypoints_t& kpts = keypoints_t() ,curses_t const& crs = curses_t(),speed_t const& vel=speed_t());

    virtual ~trajectory() {}
    virtual void                                       append        (const trajectory_ptr other) = 0;
    virtual void  append(double len,const cg::point_3& pos,const cg::quaternion& orien, optional<double> speed=boost::none) = 0;
	virtual double                                     length        () const                     = 0;
    virtual double                                     base_length   () const                     = 0;
    virtual const keypoints_t::value_type              kp_value      (double arg)                 = 0; 
    virtual curses_t::value_type                       curs_value    (double arg)                 = 0; 
    virtual boost::optional<speed_t::value_type>       speed_value   (double arg)                 = 0;
    virtual std::vector<keypoints_t::value_type>       extract_values() const                     = 0;
    virtual double                                     cur_len       () const                     = 0;
    virtual void                                       set_cur_len   (double curr_len = 0.0)      = 0;
};




inline std::vector<cg::geo_point_2> to_geo_points(const trajectory& traj)
{
    const auto vals =  traj.extract_values();
    std::vector<cg::geo_point_2> vgp2;
    for (auto it=vals.begin();it!=vals.end();++it)
    {
         vgp2.push_back(::get_base()(*it));
    }
    
    return vgp2;
}



}
