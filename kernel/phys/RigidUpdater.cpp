#include "stdafx.h"

#include "utils/high_res_timer.h"

#include "visitors/heil_visitor.h"

#include "RigidUpdater.h"
#include "BulletInterface.h"

#include "kernel/systems/systems_base.h"
#include "kernel/systems/fake_system.h"
#include "kernel/object_class.h"
#include "kernel/msg_proxy.h"

#include "aircraft.h"                         // FIXME TODO don't need here 
#include "aircraft/aircraft_common.h"
#include "aircraft/aircraft_shassis_impl.h"   // And maybe this too
#include "nm/nodes_manager.h"

#include "ada/ada.h"
#include "bada/bada_import.h"
#include "aircraft/phys_aircraft.h"
#include "vehicle.h"

#include "common/simple_route.h"
#include "flock_manager/flock_manager_common.h"

#include "object_creators.h"

// ��� ��������
#include "objects/vehicle_fwd.h"
#include "objects/vehicle.h"

#include "av/avCore/DebugRenderer.h"

#include "utils/krv_import.h"
#include "tests/systems/test_systems.h"

////  ifndef MULTITHREADED
//#define DEPRECATED_DEBUG_MODE

// FIXME
FIXME("������������ ������� ���� � ���������,���� ������ ��������")
namespace vehicle
{
	kernel::object_info_ptr create(kernel::system_ptr sys,nodes_management::manager_ptr nodes_manager,const std::string& model_name);
};

namespace aircraft
{
    kernel::object_info_ptr create(kernel::system_ptr sys,nodes_management::manager_ptr nodes_manager,phys::control_ptr        phys);
};


namespace bi
{
	struct RigidUpdater::_private
	{
		        _private()
                    : _krv_data_getter("log_minsk.txt")
#ifdef DEPRECATED_DEBUG_MODE
                    , _sys            (phys::create_phys_system())
#else
					, _sys            (nullptr/*phys::create_phys_system()*/)
#endif
                    , _msys(nullptr)
                    , _vsys(nullptr)
                    , _csys(nullptr)
                {}
#ifdef DEPRECATED
                RigidUpdater::phys_vehicles_t                          _vehicles;
#endif         
                polymorph_ptr<phys::BulletInterface>                   _sys;
                kernel::system_ptr                                     _msys;
				kernel::system_ptr                                     _vsys;
                kernel::system_ptr                                     _csys;
                kernel::msg_service                             msg_service_;
                krv::data_getter                            _krv_data_getter;
	};



	RigidUpdater::RigidUpdater( osg::Group* root, on_collision_f on_collision ) 
		: _root        (root)
		, _on_collision(on_collision)
		, _dbgDraw     (nullptr)
		, _debug       (cfg().debug.debug_drawer)
		, _last_frame_time(0)
		, selected_obj_id_(0)
		, _d(new _private)
		, _trajectory_drawer2(new TrajectoryDrawer(root,TrajectoryDrawer::LINES))
	{
        using namespace kernel;
        
        // ������ ��������� ��� ��������  
        _d->_csys = get_systems()->get_control_sys();
        _d->_msys = get_systems()->get_model_sys();

#if 0
        _trajectory_drawer2->set(_d->_krv_data_getter.kp_,cg::coloraf(1.0f,0.f,0.f,1.0f));
#endif        
      
    }

    RigidUpdater::~RigidUpdater()
    {
        delete _d;
    }

    void RigidUpdater::stopSession()
    {
        if (_d->_csys)
        {
            kernel::system_session_ptr(_d->_csys)->on_session_stopped();
            _d->_csys.reset();
        }

        if (_d->_msys)
        {
            kernel::system_session_ptr(_d->_msys)->on_session_stopped();
            _d->_msys.reset();
        }

        if (_d->_vsys)
        {
            kernel::system_session_ptr(_d->_vsys)->on_session_stopped();
            _d->_vsys.reset();
        }
    }


    void RigidUpdater::addGround( const osg::Vec3& gravity )
    {
        FIXME(Debug drawer)
#ifdef DEPRECATED
        _d->_sys->createWorld( osg::Plane(0.0f, 0.0f, 1.0f, 0.0f), gravity,
            [&](int id){
                if(_on_collision)
                    _on_collision(_physicsNodes[id].get());   
        } );
#endif
        //const bool debug( arguments.read( "--debug" ) );
        if( _debug )
        {
            //osg::notify( osg::INFO ) << "osgbpp: Debug" << std::endl;
            _dbgDraw = new avCollision::DebugRenderer();// new avCollision::GLDebugDrawer();
            _dbgDraw->setDebugMode( ~btIDebugDraw::DBG_DrawText );
            if(_root->getNumParents()>0)
                _root->getParent(0)->addChild( dynamic_cast<avCollision::DebugRenderer*>(_dbgDraw.get())->getSceneGraph() );
            else
                _root->addChild( dynamic_cast<avCollision::DebugRenderer*>(_dbgDraw.get())->getSceneGraph() );
        }

        if(_dbgDraw)
            _d->_sys->setDebugDrawer(_dbgDraw);
    }


#ifdef DEPRECATED
    void RigidUpdater::addPhysicsAirplane( osg::Node* node, const osg::Vec3& pos, const osg::Vec3& vel, double mass )
    {
        int id = _physicsNodes.size();
        
        osg::ComputeBoundsVisitor cbv;
        node->accept( cbv );
        const osg::BoundingBox& bb = cbv.getBoundingBox();

        float xm = bb.xMax() - bb.xMin();
        float ym = bb.yMax() - bb.yMin();
        float zm = bb.zMax() - bb.zMin();

        float rot_angle = -90.f;
        auto tr = osg::Matrix::translate(osg::Vec3(0.0,0.0,-(ym)/2.0f));
        if(dynamic_cast<osg::LOD*>(node))
        {
            rot_angle = 0;
            tr = osg::Matrix::translate(osg::Vec3(0,0,-(zm)/2.0f));
        }

        osg::MatrixTransform* positioned = new osg::MatrixTransform(tr);

        const osg::Quat quat(osg::inDegrees(rot_angle), osg::X_AXIS,                      
            osg::inDegrees(0.f) , osg::Y_AXIS,
            osg::inDegrees(0.f)   , osg::Z_AXIS ); 

        osg::MatrixTransform* rotated = new osg::MatrixTransform(osg::Matrix::rotate(quat));

        rotated->addChild(node);
        positioned->addChild(rotated);
        osg::Vec3 half_length ( (bb.xMax() - bb.xMin())/2.0f,(bb.zMax() - bb.zMin())/2.0f,(bb.yMax() - bb.yMin()) /2.0f );
        if(dynamic_cast<osg::LOD*>(node))
        {
            half_length = osg::Vec3 ( (bb.xMax() - bb.xMin())/2.0f,(bb.yMax() - bb.yMin())/2.0f,(bb.zMax() - bb.zMin()) /2.0f );
        }

        osg::Node*  lod3 =  findFirstNode(node,"Lod3",findNodeVisitor::not_exact);

#if 0
        _sys->createBox( id, half_length, mass );
        addPhysicsData( id, positioned, pos, vel, mass );
#else  
        _sys->createShape(lod3, id, mass);
        addPhysicsData( id, rotated, pos, vel, mass );
#endif
    }

    void RigidUpdater::addUFO(osg::Node* node,const osg::Vec3& pos, const osg::Vec3& vel, double mass)
    {
        osg::Node*  lod3 =  findFirstNode(node,"Lod3",findNodeVisitor::not_exact);
        int id = _physicsNodes.size();
        _sys->createUFO( lod3,id, mass );

        _sys->setMatrix( id, osg::Matrix::translate(pos) );
        _sys->setVelocity( id, vel );

    }

    void RigidUpdater::addUFO2(osg::Node* node,const osg::Vec3& pos, const osg::Vec3& vel, double mass)
    {
        osg::Node*  lod3 =  findFirstNode(node,"Lod3",findNodeVisitor::not_exact);
        int id = _physicsNodes.size();
        
        //nm::manager_ptr man = nm::create_manager(lod3);
        //
        //size_t pa_size = _phys_aircrafts.size();
        ////if(_phys_aircrafts.size()==0)
        //{
        //    aircraft::phys_aircraft_ptr ac_ = aircraft::phys_aircraft_impl::create(
        //                      cg::geo_base_3(cg::geo_point_3(0.000* (pa_size+1),0.0*(pa_size+1),0)),
        //                                                                           _sys,
        //                                                                            man,
        //        geo_position(cg::geo_point_3(0.000* (pa_size+1),0.005*(pa_size+1),0),cg::quaternion(cg::cpr(0, 0, 0))),
        //                                                  ada::fill_data("BADA","A319"),                                                   
        //                        boost::make_shared<aircraft::shassis_support_impl>(man),
        //                                                                             0);

        //    _phys_aircrafts.push_back(ac_);
        //}

        _aircrafts.push_back(_sys->createUFO2( lod3,id, mass ));

        //osg::ComputeBoundsVisitor cbv;
        //node->accept( cbv );
        //const osg::BoundingBox& bb = cbv.getBoundingBox();

        //float xm = bb.xMax() - bb.xMin();
        //float ym = bb.yMax() - bb.yMin();
        //float zm = bb.zMax() - bb.zMin();

        //float rot_angle = -90.f;
        //auto tr = osg::Matrix::translate(osg::Vec3(0.0,-(ym)/2.0f,0.0));
        //if(dynamic_cast<osg::LOD*>(node))
        //{
        //    rot_angle = 0;
        //    tr = osg::Matrix::translate(osg::Vec3(0,0,-(zm)/2.0f));
        //}        

        //osg::MatrixTransform* positioned = new osg::MatrixTransform(tr);

        //const osg::Quat quat(osg::inDegrees(rot_angle), osg::X_AXIS,                      
        //    osg::inDegrees(0.f) , osg::Y_AXIS,
        //    osg::inDegrees(0.f)   , osg::Z_AXIS ); 

        //osg::MatrixTransform* rotated = new osg::MatrixTransform(osg::Matrix::rotate(quat));
        //positioned->addChild(rotated);
        //rotated->addChild(node);

        addPhysicsData( id, addGUIObject(node), pos, /*vel*/osg::Vec3(0.0,0.0,0.0), mass );
        phys::aircraft::control_ptr(_aircrafts.back())->apply_force(from_osg_vector3(vel));
    }


    void RigidUpdater::addUFO3(osg::Node* node,const osg::Vec3& pos, const osg::Vec3& vel, double mass)
    {
        // TODO FIXME � ��� ����� �������� ������� ���������� ����� ������ �����
#if  0  // Under construction
        //osg::Node*  lod0 =  findFirstNode(node,"Lod0",findNodeVisitor::not_exact);
        osg::Node*  lod3 =  findFirstNode(node,"Lod3",findNodeVisitor::not_exact);
        osg::Node*  root =  findFirstNode(node,"root",findNodeVisitor::not_exact);
        size_t object_id = 0;
        root->getUserValue("id",object_id);

        int id = _physicsNodes.size();

        nm::manager_ptr man = nm::create_manager(lod3);

        aircraft::shassis_support_ptr s = boost::make_shared<aircraft::shassis_support_impl>(nm::create_manager(node));

        size_t pa_size = _phys_aircrafts.size();
        aircraft::phys_aircraft_ptr ac_ = aircraft::phys_aircraft_impl::create(
            get_base(),
            _sys,
            man,
            geo_position(cg::geo_point_3(0.000,0.002*((double)pa_size+.1),0),cg::quaternion(cg::cpr(90, 0, 0))),
            ada::fill_data("BADA","A319"),                                                   
            s,
            0);


        _phys_aircrafts.emplace_back(boost::make_shared<aircraft::model>(nm::create_manager(node),ac_,s));
        _sys->registerBody(id,ac_->get_rigid_body());
        

        //_phys_aircrafts.back().shassis->visit_groups([=](aircraft::shassis_group_t & shassis_group)
        //{
        //    //if (to_be_opened)
        //    //    shassis_group.open(true);
        //    //else 
        //        if (!shassis_group.is_front)
        //                shassis_group.close(false);
        //});

        //addPhysicsData( id, positioned, pos, /*vel*/osg::Vec3(0.0,0.0,0.0), mass );
        osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
        mt->setName("phys_ctrl");
        mt->setUserValue("id",object_id);
        mt->addChild( node );

        _root->addChild( mt.get() );

        _physicsNodes[id] = mt;
#endif
    }

    void RigidUpdater::addUFO4(osg::Node* node,const osg::Vec3& pos, const osg::Vec3& vel, double mass)
    {
        // TODO FIXME � ��� ����� �������� ������� ���������� ����� ������ �����

        //osg::Node*  lod0 =  findFirstNode(node,"Lod0",findNodeVisitor::not_exact);
        osg::Node*  lod3 =  findFirstNode(node,"Lod3",findNodeVisitor::not_exact);
        osg::Node*  root =  findFirstNode(node,"root",findNodeVisitor::not_exact);
        size_t object_id = 0;
        root->getUserValue("id",object_id);

        int id = _physicsNodes.size();

        nm::manager_ptr man = nm::create_manager(_d->_msys,dict_t(),lod3);

        FIXME("� ��� � ������ � ������")
        //aircraft::shassis_support_ptr s = boost::make_shared<aircraft::shassis_support_impl>(nm::create_manager(node));

        size_t pa_size = _model_aircrafts.size();

        _model_aircrafts.emplace_back(aircraft::create(_d->_msys,man,_sys) /*boost::make_shared<aircraft::model>(nm::create_manager(node),ac_,s)*/);
        _sys->registerBody(id);  // FIXME ��������� ������ ������ //_sys->registerBody(id,ac_->get_rigid_body());

        osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
        mt->setName("phys_ctrl");
        mt->setUserValue("id",object_id);

        mt->addChild( node );

        _root->addChild( mt.get() );

        _physicsNodes[id] = mt;
    }

    void RigidUpdater::addVehicle(osg::Node* node,const osg::Vec3& pos, const osg::Vec3& vel, double mass)
    {           
        // TODO FIXME � ��� ����� �������� ������� ���������� ����� ������ �����

        //osg::Node*  lod0 =  findFirstNode(node,"Lod0",findNodeVisitor::not_exact);
        osg::Node*  lod3 =  findFirstNode(node,"Lod3",findNodeVisitor::not_exact);
        osg::Node*  root =  findFirstNode(node,"root",findNodeVisitor::not_exact);
        size_t object_id = 0;
        root->getUserValue("id",object_id);

        int id = _physicsNodes.size();
        phys::ray_cast_vehicle::info_ptr veh = _sys->createVehicle(node/*lod3?lod3:node*/,id,mass);
        
        FIXME("� ������ �������� lod3 ����� �������� ������, ���� �� ����")
        // FIXME
        //if(lod3) 
        //    lod3->setNodeMask(0);

        //_sys->registerBody(id,phys::rigid_body_impl_ptr(veh)->get_body());
        
        _d->_vehicles.emplace_back(veh);

        //addPhysicsData( id, positioned, pos, /*vel*/osg::Vec3(0.0,0.0,0.0), mass );
        osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
        mt->setName("phys_ctrl");
        mt->setUserValue("id",object_id);
        mt->addChild( /*addGUIObject_v(node)*/node );
        _root->addChild( mt.get() );

        _physicsNodes[id] = mt;

        _sys->setMatrix( id, osg::Matrix::translate(pos) );

        //veh->set_steer(10);
    }

    void RigidUpdater::addVehicle2(const string& model_name,osg::Node* node,const osg::Vec3& pos, const osg::Vec3& vel, double mass)
    {           
        int id = _physicsNodes.size();
        osg::Node*  lod3 =  findFirstNode(node,"Lod3",findNodeVisitor::not_exact);
        osg::Node*  root =  findFirstNode(node,"root",findNodeVisitor::not_exact); 
        size_t object_id = 0;
        root->getUserValue("id",object_id);

#ifdef  OSG_NODE_IMPL
        lod3?lod3->setNodeMask(0):0;
        nm::manager_ptr man = nm::create_manager(_d->_msys,dict_t(),node);
        //lod3?lod3->setNodeMask(0xffffffff):0;
#else
        nm::manager_ptr man = nm::create_manager(_d->_msys,dict_t(),nullptr/*lod3?lod3:node*/);
        object_id  = man->get_node(0)->object_id();
#endif

        FIXME("�������")
        man->set_model(model_name);

        _phys_vehicles.push_back(vehicle::create(_d->_msys,man,model_name));
        _sys->registerBody(id);  // FIXME ��������� ������ ������ 
        //man->set_model(model_name);

        osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
        mt->setName("phys_ctrl");
        mt->setUserValue("id",object_id);
        
        mt->addChild( /*addGUIObject_v(node)*/node );


        _root->addChild( mt.get() );

        _physicsNodes[id] = mt;

        _sys->setMatrix( id, osg::Matrix::translate(pos) );
        osg::Matrix mmm = _sys->getMatrix(id);
        _phys_vehicles.back()->set_state_debug(vehicle::state_t(cg::geo_base_2()(from_osg_vector3(pos)), from_osg_quat(mmm.getRotate()).get_course(), 0.0f));

        //veh->set_steer(10);

    }
#endif

    void RigidUpdater::addPhysicsBox( osg::Box* shape, const osg::Vec3& pos, const osg::Vec3& vel, double mass )
    {
        int id = _physicsNodes.size();
        _d->_sys->createBox( id, shape->getHalfLengths(), mass );
        addPhysicsData( id, shape, pos, vel, mass );
    }

    void RigidUpdater::addPhysicsSphere( osg::Sphere* shape, const osg::Vec3& pos, const osg::Vec3& vel, double mass )
    {
        int id = _physicsNodes.size();
        _d->_sys->createSphere( id, shape->getRadius(), mass );
        addPhysicsData( id, shape, pos, vel, mass );
    }

    struct frame_timer
    {
        inline frame_timer(osgViewer::View* view,double& last_frame_time)
            : last_frame_time(last_frame_time)
            , current_time(0)
        {
           current_time = view->getFrameStamp()->getSimulationTime();
        }

        inline double diff()
        {
          return current_time - last_frame_time ; 
        }

        inline ~frame_timer()
        {
           last_frame_time  = current_time;
        }

    private:
        double  current_time;
        double& last_frame_time;
    };
                                                

    bool RigidUpdater::handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa )
    {
        osgViewer::View* view = static_cast<osgViewer::View*>( &aa );
        frame_timer ftm (view,_last_frame_time);
        if ( !view || !_root ) return false;

#ifdef DEPRECATED_DEBUG_MODE
        switch ( ea.getEventType() )
        {
        case osgGA::GUIEventAdapter::KEYUP:
            if ( ea.getKey()==osgGA::GUIEventAdapter::KEY_Return )
            {
                osg::Vec3 eye, center, up, dir;
                view->getCamera()->getViewMatrixAsLookAt( eye, center, up );
                dir = center - eye; dir.normalize();
                addPhysicsSphere( new osg::Sphere(osg::Vec3(), 0.5f), eye, dir * 200.0f, 2.0 );
            } 
            else if ( ea.getKey()==osgGA::GUIEventAdapter::KEY_Right )
            {
#ifdef DEPRECATED
                double steer  = phys::aircraft::control_ptr(_aircrafts[0])->get_steer();
                steer = cg::bound(cg::norm180(/*desired_course - cur_course*/++steer),-65., 65.);
                phys::aircraft::control_ptr(_aircrafts[1])->set_steer(steer);  
#endif
            }
            else if ( ea.getKey()==osgGA::GUIEventAdapter::KEY_Left )
            {
#ifdef DEPRECATED
                double steer  = phys::aircraft::control_ptr(_aircrafts[0])->get_steer();
                steer = cg::bound(cg::norm180(/*desired_course - cur_course*/--steer),-65., 65.);
                phys::aircraft::control_ptr(_aircrafts[1])->set_steer(steer);
#endif
            }
            else if ( ea.getKey()==osgGA::GUIEventAdapter::KEY_O )
            {

                auto vvv = kernel::find_object<aircraft::control_ptr>(kernel::object_collection_ptr(_d->_csys).get(),"aircraft 0");
                if(vvv)
                {
                      aircraft::aircraft_ipo_control_ptr(vvv)->set_malfunction(aircraft::MF_FIRE_ON_BOARD,true); 
                      aircraft::aircraft_ipo_control_ptr(vvv)->set_malfunction(aircraft::MF_ONE_ENGINE,true); 
                }

            }
            else if ( ea.getKey()==osgGA::GUIEventAdapter::KEY_M /*&& (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT)*/)
            {
 // FIXME ��������� � ������
#if 0
                _phys_aircrafts.back().get_chassis()->visit_groups([=](aircraft::shassis_group_t & shassis_group)
                {
                    //if (to_be_opened)
                    //    shassis_group.open(true);
                    //else
                    if (!shassis_group.is_front)
                        shassis_group.close(false);
                });
#endif
                auto vvv = kernel::find_object<vehicle::control_ptr>(kernel::object_collection_ptr(_d->_csys).get(),"vehicle 0");
                if(vvv)
                {
                    vvv->attach_tow();      
                }
            }
            else if ( ea.getKey()==osgGA::GUIEventAdapter::KEY_N /*&& (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT)*/)
            {
                auto vvv = kernel::find_object<vehicle::control_ptr>(kernel::object_collection_ptr(_d->_csys).get(),"vehicle 0");
                if(vvv)
                {
                    vvv->detach_tow();
                }
            }
            else if ( ea.getKey()==osgGA::GUIEventAdapter::KEY_K )
            {
                 auto vvv = kernel::find_object<vehicle::control_ptr>(kernel::object_collection_ptr(_d->_csys).get(),"vehicle 0");
                 auto sr_obj = kernel::find_object<simple_route::control_ptr>(kernel::object_collection_ptr(_d->_csys).get(),"simple_route 0");
                 if(sr_obj)
                 {
                    sr_obj->set_speed(5);
                    vvv->follow_route("simple_route 0");
                 }
                 
            } 
            else if ( ea.getKey()==osgGA::GUIEventAdapter::KEY_T )
            {
                auto vvv = kernel::find_object<vehicle::control_ptr>(kernel::object_collection_ptr(_d->_csys).get(),"vehicle 0");
                if(vvv)
                {
                    vvv->follow_trajectory("simple_route 0");
                }

            }
            else if ( ea.getKey()==osgGA::GUIEventAdapter::KEY_B )
            {
                auto vvv = kernel::find_object<vehicle::control_ptr>(kernel::object_collection_ptr(_d->_csys).get(),"vehicle 0");
                vvv->set_brake(0.35);
            }
            else if ( ea.getKey()==osgGA::GUIEventAdapter::KEY_Up )
            {
                const kernel::object_collection  *  col = dynamic_cast<kernel::object_collection *>(_d->_msys.get());
                
                kernel::visit_objects<aircraft::model_control_ptr>(col,[this](aircraft::model_control_ptr a)->bool
                {
                    auto nm = kernel::find_first_child<nodes_management::manager_ptr>(a);
                    uint32_t nm_id = kernel::object_info_ptr(nm)->object_id();

                    if( nm_id == this->selected_obj_id_)
                    {
                        const double ras = aircraft::model_info_ptr(a)->rotors_angular_speed() + 10;
                        a->set_rotors_angular_speed( ras>250 ? 250 : ras);
                        return false;
                    }
                    return true;
                });

            }  
            else if ( ea.getKey()==osgGA::GUIEventAdapter::KEY_Down )
            {
                const kernel::object_collection  *  col = dynamic_cast<kernel::object_collection *>(_d->_msys.get());

                kernel::visit_objects<aircraft::model_control_ptr>(col,[this](aircraft::model_control_ptr a)->bool
                {
                    auto nm = kernel::find_first_child<nodes_management::manager_ptr>(a);
                    uint32_t nm_id = kernel::object_info_ptr(nm)->object_id();

                    if( nm_id == this->selected_obj_id_)
                    {
                        const double ras = aircraft::model_info_ptr(a)->rotors_angular_speed() - 10;
                        a->set_rotors_angular_speed( ras>0? ras : 0);
                        return false;
                    }
                    return true;
                });


            }
            break;
        case osgGA::GUIEventAdapter::FRAME:
            {
                double dt = _hr_timer.set_point();
                double dt1 = ftm.diff();

               if (abs(dt-dt1)>0.1)
               { 
                    force_log fl;
                    LOG_ODS_MSG( "Simulation time differ from real time more the 0.1 sec  " << dt  << "  " <<  dt1 << "\n");
               }

                if( _dbgDraw)
                    _dbgDraw->BeginDraw();

#ifdef DEPRECATED
                for(auto it = _model_aircrafts.begin();it!=_model_aircrafts.end();++it)
                {   
                    aircraft::int_control_ptr(*it)->update( view->getFrameStamp()->getSimulationTime()/*dt*/ );
                } 

                for(auto it = _phys_vehicles.begin();it!=_phys_vehicles.end();++it)
                {   
                          (*it)->update( view->getFrameStamp()->getSimulationTime()/*dt*/ );
                }   
#endif                 

                for ( NodeMap::iterator itr=_physicsNodes.begin();
                    itr!=_physicsNodes.end(); ++itr )
                {
                    osg::Matrix matrix = _d->_sys->getMatrix(itr->first);
                    itr->second->setMatrix( matrix );
                }

                if( _dbgDraw)
                {
                    _d->_sys->debugDrawWorld();
                    _dbgDraw->EndDraw();
                }
            } 

            break;

        default: break;
        }
#endif

        return false;
    }


    void RigidUpdater::addPhysicsData( int id, osg::Shape* shape, const osg::Vec3& pos,
        const osg::Vec3& vel, double /*mass*/ )
    {
        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable( new osg::ShapeDrawable(shape) );

        osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
        mt->addChild( geode.get() );
        _root->addChild( mt.get() );

        _d->_sys->setMatrix( id, osg::Matrix::translate(pos) );
        _d->_sys->setVelocity( id, vel );
        _physicsNodes[id] = mt;
    }

    void RigidUpdater::addPhysicsData( int id, osg::Node* node, const osg::Vec3& pos,
        const osg::Vec3& vel, double /*mass*/ )
    {
        osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
        mt->addChild( node );
        _root->addChild( mt.get() );

        _d->_sys->setMatrix( id, osg::Matrix::translate(pos) );
        _d->_sys->setVelocity( id, vel );
        _physicsNodes[id] = mt;
    }


    void RigidUpdater::handleSelectObjectEvent(uint32_t id )
    {
         selected_obj_id_ = id;

#ifdef DEPRECATED
         auto it_am = std::find_if(_model_aircrafts.begin(),_model_aircrafts.end(),[this](aircraft::info_ptr amp)->bool
         {
             if(amp->root() && amp->root()->object_id()==this->selected_obj_id_)
                 return true;

             return false;
         });

         if(it_am!=_model_aircrafts.end())
         {
            auto traj = aircraft::int_control_ptr(*it_am)->get_trajectory();
            if (traj) _trajectory_drawer->set(traj);
         } 
#endif
         bool a_or_v = false;

         const kernel::object_collection  *  col = dynamic_cast<kernel::object_collection *>(_d->_csys.get());

         kernel::visit_objects<vehicle::control_ptr>(col,[this,&a_or_v](vehicle::control_ptr a)->bool
         {
             auto nm = kernel::find_first_child<nodes_management::manager_ptr>(a);
             uint32_t nm_id = kernel::object_info_ptr(nm)->object_id();

             if( nm_id == this->selected_obj_id_)
             {
                 selected_object_type_signal_(VEHICLE_TYPE);
                 auto traj = a->get_trajectory();
                 if (traj) _trajectory_drawer->set(traj,cg::coloraf(0.0f,1.f,0.f,1.0f));
                 a_or_v = true;
                 return false;
             }
             return true;
         });

         kernel::visit_objects<aircraft::control_ptr>(col,[this,&a_or_v](aircraft::control_ptr a)->bool
         {
             auto nm = kernel::find_first_child<nodes_management::manager_ptr>(a);
             uint32_t nm_id = kernel::object_info_ptr(nm)->object_id();

             if( nm_id == this->selected_obj_id_)
             {
                 selected_object_type_signal_(AIRCRAFT_TYPE);
                 auto traj = aircraft::int_control_ptr(a)->get_trajectory();
                 if (traj) _trajectory_drawer->set(traj,cg::coloraf(1.0f,0.0f,0.f,1.0f));
                 a_or_v = true;
                 return false;
             }
             return true;
         });

         //kernel::visit_objects<aircraft_physless::control_ptr>(col,[this,&a_or_v](aircraft_physless::control_ptr a)->bool
         //{
         //    auto nm = kernel::find_first_child<nodes_management::manager_ptr>(a);
         //    uint32_t nm_id = kernel::object_info_ptr(nm)->object_id();

         //    if( nm_id == this->selected_obj_id_)
         //    {
         //        selected_object_type_signal_(AIRCRAFT_TYPE);
         //        auto traj = aircraft::int_control_ptr(a)->get_trajectory();
         //        if (traj) _trajectory_drawer->set(traj,cg::coloraf(1.0f,0.0f,0.f,1.0f));
         //        a_or_v = true;
         //        return false;
         //    }
         //    return true;
         //});

         if(!a_or_v)
            selected_object_type_signal_(NONE_TYPE);
         
    }

    FIXME(����� �� ������ ����? �� �����.)
    void RigidUpdater::handlePointEvent(std::vector<cg::point_3> const &cartesian_simple_route)
    {   
        decart_position target_pos;
        target_pos.pos = cartesian_simple_route.back();
        geo_position gp(target_pos, ::get_base());
        
        if(selected_obj_id_)
        {

#ifdef DEPRECATED             
            auto it_am = std::find_if(_model_aircrafts.begin(),_model_aircrafts.end(),[this](aircraft::info_ptr amp)->bool
            {
                if(amp->root() && amp->root()->object_id()==this->selected_obj_id_)
                    return true;

                return false;
            });
            
            if(it_am!=_model_aircrafts.end())
            {
                aircraft::info_ptr am=*it_am;
                decart_position cur_pos = aircraft::int_control_ptr(am)->get_local_position();
                target_pos.orien = cg::cpr(cg::polar_point_2(target_pos.pos - cur_pos.pos).course,0,0);

                if (!aircraft::int_control_ptr(am)->get_trajectory())
                {
                     aircraft::int_control_ptr(am)->set_trajectory( fms::trajectory::create(cur_pos,target_pos,aircraft::min_radius(),aircraft::step()));
                }
                else
                {  
                    fms::trajectory_ptr main_ = aircraft::int_control_ptr(am)->get_trajectory();

                    decart_position begin_pos(cg::point_3(main_->kp_value(main_->length()),0)
                                             ,cg::cpr(main_->curs_value(main_->length()),0,0) );
            
                    target_pos.orien = cg::cpr(cg::polar_point_2(target_pos.pos - begin_pos.pos).course);
			
                    fms::trajectory_ptr traj = fms::trajectory::create ( begin_pos,
                                                                         target_pos,
                                                                         aircraft::min_radius(),
                                                                         aircraft::step());

                    main_->append(traj);
                }
       
                // ��������� ���������
                _trajectory_drawer->set(aircraft::int_control_ptr(am)->get_trajectory());

                auto vgp2 = fms::to_geo_points(*(aircraft::int_control_ptr(am)->get_trajectory().get()));
            }
            else
            {
                auto it_vh = std::find_if(_phys_vehicles.begin(),_phys_vehicles.end(),[this](vehicle::model_base_ptr vh)->bool
                {
                    if(vh->get_root() && vh->get_root()->object_id()==this->selected_obj_id_)
                        return true;

                    return false;
                });

                if(it_vh!=_phys_vehicles.end())
                    (*it_vh)->go_to_pos(gp.pos,90);
            }
#endif            
            const kernel::object_collection  *  col = dynamic_cast<kernel::object_collection *>(_d->_csys.get());
#if 0  // ������� ��� �����           
            kernel::visit_objects<vehicle::control_ptr>(col,[this,&gp](vehicle::control_ptr a)->bool
            {
				auto nm = kernel::find_first_child<nodes_management::manager_ptr>(a);
				uint32_t nm_id = kernel::object_info_ptr(nm)->object_id();

                if( nm_id == this->selected_obj_id_)
                {
                     a->goto_pos(gp.pos,90);
                     return false;
                }
                return true;
            });
#endif

            kernel::visit_objects<vehicle::control_ptr>(col,[this,&gp, &target_pos](vehicle::control_ptr vc)->bool
            {
                auto nm = kernel::find_first_child<nodes_management::manager_ptr>(vc);
                uint32_t nm_id = kernel::object_info_ptr(nm)->object_id();

                if( nm_id == this->selected_obj_id_)
                {

#ifdef USING_SIMPLE_ROUTE
                    auto sr_obj = kernel::find_first_object<simple_route::control_ptr>(kernel::object_collection_ptr(_d->_csys).get());
                    sr_obj->add_point(gp.pos);
#else
                    decart_position cur_pos = vc->get_local_position();
                    target_pos.orien = cg::cpr(cg::polar_point_2(target_pos.pos - cur_pos.pos).course,0,0);

                    if (!vc->get_trajectory())
                    {
                        vc->set_trajectory( fms::trajectory::create(cur_pos,target_pos,aircraft::min_radius(),aircraft::step()));
                    }
                    else
                    {  
                        fms::trajectory_ptr main_ = vc->get_trajectory();

                        decart_position begin_pos(cg::point_3(main_->kp_value(main_->length()),0)
                            ,main_->curs_value(main_->length()).cpr()/*cg::cpr(main_->curs_value(main_->length()),0,0)*/ );

                        target_pos.orien = cg::cpr(cg::polar_point_2(target_pos.pos - begin_pos.pos).course);

                        fms::trajectory_ptr traj = fms::trajectory::create( begin_pos,
                            target_pos,
                            aircraft::min_radius(),
                            aircraft::step());

                        main_->append(traj);

                        vc->set_trajectory(main_);
                    }
#endif

                    return false;
                }
                return true;
            });

            kernel::visit_objects<aircraft::control_ptr>(col,[this,&target_pos](aircraft::control_ptr a)->bool
            {
                auto nm = kernel::find_first_child<nodes_management::manager_ptr>(a);
                uint32_t nm_id = kernel::object_info_ptr(nm)->object_id();

                if( nm_id == this->selected_obj_id_)
                {
                    aircraft::info_ptr am= aircraft::info_ptr(a);

                    decart_position cur_pos = aircraft::int_control_ptr(am)->get_local_position();
                    target_pos.orien = cg::cpr(cg::polar_point_2(target_pos.pos - cur_pos.pos).course,0,0);

                    if (!aircraft::int_control_ptr(am)->get_trajectory())
                    {
                        aircraft::int_control_ptr(am)->set_trajectory( fms::trajectory::create(cur_pos,target_pos,aircraft::min_radius(),aircraft::step()));
                    }
                    else
                    {  
                        fms::trajectory_ptr main_ = aircraft::int_control_ptr(am)->get_trajectory();

                        decart_position begin_pos(cg::point_3(main_->kp_value(main_->length()),0)
                            ,main_->curs_value(main_->length()).cpr()/*cg::cpr(main_->curs_value(main_->length()),0,0)*/ );

                        target_pos.orien = cg::cpr(cg::polar_point_2(target_pos.pos - begin_pos.pos).course);

                        fms::trajectory_ptr traj = fms::trajectory::create( begin_pos,
                            target_pos,
                            aircraft::min_radius(),
                            aircraft::step());

                        main_->append(traj);

                        aircraft::int_control_ptr(am)->set_trajectory(main_);
                    }

                    // ��������� ���������
                    _trajectory_drawer->set(aircraft::int_control_ptr(am)->get_trajectory(),cg::coloraf(1.0f,0.0f,0.0f,1.0f));

                    return false;
                }
                return true;
            });


            //kernel::visit_objects<aircraft_physless::control_ptr>(col,[this,&target_pos](aircraft_physless::control_ptr a)->bool
            //{
            //    auto nm = kernel::find_first_child<nodes_management::manager_ptr>(a);
            //    uint32_t nm_id = kernel::object_info_ptr(nm)->object_id();

            //    if( nm_id == this->selected_obj_id_)
            //    {
            //        aircraft_physless::info_ptr am= aircraft_physless::info_ptr(a);

            //        decart_position cur_pos = aircraft::int_control_ptr(am)->get_local_position();
            //        target_pos.orien = cg::cpr(cg::polar_point_2(target_pos.pos - cur_pos.pos).course,0,0);

            //        if (!aircraft::int_control_ptr(am)->get_trajectory())
            //        {
            //            aircraft::int_control_ptr(am)->set_trajectory( fms::trajectory::create(cur_pos,target_pos,aircraft::min_radius(),aircraft::step()));
            //        }
            //        else
            //        {  
            //            fms::trajectory_ptr main_ = aircraft::int_control_ptr(am)->get_trajectory();

            //            decart_position begin_pos(cg::point_3(main_->kp_value(main_->length()),0)
            //                ,main_->curs_value(main_->length()).cpr()/*cg::cpr(main_->curs_value(main_->length()),0,0)*/ );

            //            target_pos.orien = cg::cpr(cg::polar_point_2(target_pos.pos - begin_pos.pos).course);

            //            fms::trajectory_ptr traj = fms::trajectory::create( begin_pos,
            //                target_pos,
            //                aircraft::min_radius(),
            //                aircraft::step());

            //            main_->append(traj);

            //            aircraft::int_control_ptr(am)->set_trajectory(main_);
            //        }

            //        // ��������� ���������
            //        _trajectory_drawer->set(aircraft::int_control_ptr(am)->get_trajectory(),cg::coloraf(1.0f,0.0f,0.0f,1.0f));

            //        return false;
            //    }
            //    return true;
            //});

        }

        // _trajectory_drawer->set(simple_route);

        //_phys_aircrafts[0].aircraft->go_to_pos(gp.pos ,gp.orien);
        
    }

}
