#include "stdafx.h"

#include "kernel/systems/vis_system.h"

#include "visual_object_impl.h"
#include "av/Scene.h"


namespace kernel
{
    //! ����������� ����������� �������
    visual_object_impl::visual_object_impl( std::string const & res, uint32_t seed )
        : scene_( avScene::GetScene() )
        , loaded_(false)
    {
        scene_->subscribe_object_loaded(boost::bind(&visual_object_impl::object_loaded,this,_1));
        node_ = scene_->load(res, nullptr, seed);

        // root_ = findFirstNode(node_,"root",findNodeVisitor::not_exact);

        // node_->setNodeMask(0);
    }

    visual_object_impl::visual_object_impl(  nm::node_control_ptr parent, std::string const & res, uint32_t seed )
                        : scene_ ( avScene::GetScene() )
                        , parent_(parent)
                        , loaded_(false)
    {
         
        scene_->subscribe_object_loaded(boost::bind(&visual_object_impl::object_loaded,this,_1));

        auto const& vn = nm::vis_node_control_ptr(parent)->vis_nodes();
        node_ = scene_->load(res,vn.size()>0?vn[0]:nullptr, seed);
        // root_ = findFirstNode(node_,"root",findNodeVisitor::not_exact);
    }

    visual_object_impl::~visual_object_impl()
    {
        scene_->removeChild(node_.get());

        // scene_->get_objects()->remove(node_.get());
    }
    
    void visual_object_impl::object_loaded( uint32_t seed )
    {
         if(seed_==seed)
         {
           root_ = findFirstNode(node_,"root",findNodeVisitor::not_exact); 
           loaded_ = true;
           object_loaded_signal_(seed);
         }
    }

    osg::ref_ptr<osg::Node> visual_object_impl::node() const
    {
        return node_;
    }
    
    osg::ref_ptr<osg::Node> visual_object_impl::root() const
    {
        return root_;
    }
    
    void visual_object_impl::set_visible(bool visible)
    {
        if(loaded_)
            node_->setNodeMask(visible?0xffffffff:0);
    }
}
