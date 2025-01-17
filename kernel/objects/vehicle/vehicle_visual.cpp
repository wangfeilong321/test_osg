#include "stdafx.h"
#include "precompiled_objects.h"

#include "vehicle_visual.h"
#include "common/text_label.h"


namespace vehicle
{
    struct visual::tow_support
    {
        tow_support(visual_object_ptr obj,const std::string& tow_type )
           : tow_visual_object_(obj)
           , body_s_(nullptr) 
        {
            if(tow_type=="towbar")
            {   
                init_towbar();
                update_f_ = std::bind(&tow_support::towbar_update,this,sp::_1,sp::_2);
            }
            else
                update_f_ = std::bind(&tow_support::tube_update,this,sp::_1,sp::_2);
        }

        inline void update( cg::polar_point_3f const  &dir, point_3f offset )
        {
                update_f_(dir,offset);
        }

    private:

        inline void init_towbar()
        {
            body_s_ = findFirstNode((tow_visual_object_)->root(),"body_s");
            body_b_ = findFirstNode((tow_visual_object_)->root(),"body_b");
            body_a_ = findFirstNode((tow_visual_object_)->root(),"body_a");

            osg::ComputeBoundsVisitor cbvs;
            body_s_->accept( cbvs );
            const osg::BoundingBox bb_s = cbvs.getBoundingBox();

            osg::ComputeBoundsVisitor cbvb;
            body_b_->accept( cbvb );
            const osg::BoundingBox bb_b = cbvb.getBoundingBox();

            osg::ComputeBoundsVisitor cbva;
            body_a_->accept( cbva );
            const osg::BoundingBox bb_a = cbva.getBoundingBox();

            osg::ComputeBoundsVisitor cbv;
            (tow_visual_object_)->root()->accept( cbv );
            const osg::BoundingBox bb_ = cbv.getBoundingBox();

            radius_   = abs(bb_.yMax() - bb_.yMin())/2.0;//(*tow_visual_object_)->root()->getBound().radius();

            radius_s_ = abs(bb_s.yMax() - bb_s.yMin())/2.0;//body_s_->getBound().radius();
            radius_a_ = abs(bb_a.yMax() - bb_a.yMin())/2.0;//body_a_->getBound().radius(); 
            radius_b_ = abs(bb_b.yMax() - bb_b.yMin())/2.0;//body_b_->getBound().radius();
        }

        void towbar_update( cg::polar_point_3f const  &dir, point_3f offset )
        {
            const double scale_coeff = (dir.range -  (radius_a_ + radius_b_)*2) / radius_s_ *.5; 

            osg::Matrix trMatrix;            
            trMatrix.setTrans(to_osg_vector3(offset));
            trMatrix.setRotate(osg::Quat(osg::inDegrees(-dir.course), osg::Z_AXIS));
            osg::Matrix scaleMatrix;
            FIXME("All good, but 1.1")
            scaleMatrix.makeScale(1., scale_coeff * 1.1 , 1.);


            // (*tow_visual_object_)->root()->asTransform()->asMatrixTransform()->setMatrix(scaleMatrix);
            {
                const double dy = (scale_coeff - 1) * radius_s_;
                osg::Matrix trMatrix;            
                trMatrix.setTrans(osg::Vec3(0,-(0/*dir.range *.5 - radius_*/),0));
                body_b_->asTransform()->asMatrixTransform()->setMatrix(trMatrix);
                trMatrix.setTrans(osg::Vec3(0,2*dy/*dir.range *.5 - radius_*/,0));
                body_a_->asTransform()->asMatrixTransform()->setMatrix(trMatrix);
                osg::Matrix m3;
                m3.setTrans(osg::Vec3(0,-(0.05*dy/*dir.range *.5 - radius_*/),0));
                scaleMatrix.postMult(m3);
                body_s_->asTransform()->asMatrixTransform()->setMatrix(scaleMatrix);
            }

            (tow_visual_object_)->node()->asTransform()->asMatrixTransform()->setMatrix(trMatrix);
        }

        void tube_update(cg::polar_point_3f const  &dir, point_3f offset )
        {
            osg::Matrix trMatrix;
            trMatrix.setTrans(to_osg_vector3(offset));
            trMatrix.setRotate(osg::Quat(osg::inDegrees(-dir.course), osg::Z_AXIS));
            osg::Matrix scaleMatrix;
            scaleMatrix.makeScale(1., dir.range, 1.);

            (tow_visual_object_)->root()->asTransform()->asMatrixTransform()->setMatrix(scaleMatrix);
            (tow_visual_object_)->node()->asTransform()->asMatrixTransform()->setMatrix(trMatrix);
        }

        double                radius_;
        double                radius_s_;
        double                radius_a_;
        double                radius_b_;
        osg::Node *           body_s_;
        osg::Node *           body_a_;
        osg::Node *           body_b_;

        visual_object_ptr     tow_visual_object_;
        typedef std::function<void(cg::polar_point_3f const &dir, point_3f offset )>  update_t;
        update_t              update_f_; 
    };
}


namespace vehicle
{

object_info_ptr visual::create(object_create_t const& oc, dict_copt dict)
{
    return object_info_ptr(new visual(oc, dict));
}

AUTO_REG_NAME(vehicle_visual, visual::create);

visual::visual(object_create_t const& oc, dict_copt dict)
    : view(oc, dict)
{
    visual_system* vsys = dynamic_cast<visual_system*>(sys_);

#ifndef ASYNC_OBJECT_LOADING 
    label_object_ = vsys->create_visual_object(nm::node_control_ptr(root_),"text_label.scg");
    ls_ = boost::make_shared<visual_objects::label_support>(label_object_, settings_.custom_label);
#endif
}

void visual::settings_changed()
{
    view::settings_changed();
    ls_->set_text(settings_.custom_label);
}

void visual::update(double time)
{
    view::update(time);
    
    if (nodes_management::vis_node_info_ptr(root_)->is_visible() && aerotow_)
    {

        if (!tow_visual_object_)
        {

            std::string tow_type = "towbar"; // "towbar"  "tube"
            tow_visual_object_ = dynamic_cast<visual_system*>(sys_)->create_visual_object(tow_type) ;
            
            ts_ = boost::make_shared<tow_support>(*tow_visual_object_, tow_type);
        }


        aircraft::info_ptr towair;

        if (tow_visual_object_ && *tow_visual_object_)
        {
            geo_base_3 base = dynamic_cast<visual_system_props*>(sys_)->vis_props().base_point;
            
            nodes_management::node_info_ptr aerotow_root = aerotow_->root();
            
            quaternion atr_quat = aerotow_root->get_global_orien();

            point_3f offset = base(/*tow_point_node_*/current_tow_point_node_->get_global_pos());
            geo_point_3 air_tow_pos = geo_base_3(aerotow_root->get_global_pos())(aerotow_root->get_global_orien().rotate_vector(point_3(aerotow_->tow_point_transform().translation())));
            point_3f offset2 = base(air_tow_pos);

            cg::polar_point_3f dir(offset2 - offset);
            cpr orien(dir.course, dir.pitch, 0);

            // cg::transform_4f tr(cg::as_translation(offset), rotation_3f(orien), cg::as_scale(point_3f(1., dir.range, 1.)));
            // (*tow_visual_object_)->node()->as_transform()->set_transform(tr);
            
            ts_->update(dir, offset);

            (*tow_visual_object_)->set_visible(true);
        }
    }
    else
    {
        if (tow_visual_object_ && *tow_visual_object_)
            (*tow_visual_object_)->set_visible(false);
    }
}


} // vehicle
