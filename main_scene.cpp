// test_osg.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "creators.h"
#include "animation_handler.h"
#include "find_node_visitor.h" 
#include "info_visitor.h"
#include "SkyBox.h"

#include <osgEphemeris/EphemerisModel.h>

#define TEST_EPHEMERIS
#define TEST_PRECIP
// #define TEST_NODE_TRACKER
// #define TEST_SKYBOX

osg::Matrix computeTargetToWorldMatrix( osg::Node* node ) // const
{
    osg::Matrix l2w;
    if ( node && node->getNumParents()>0 )
    {
        osg::Group* parent = node->getParent(0);
        l2w = osg::computeLocalToWorld( parent->
            getParentalNodePaths()[0] );
    }
    return l2w;
}



void AddLight( osg::ref_ptr<osg::MatrixTransform> rootnode ) 
{

    
    osg::Node* light0 = effects::createLightSource(
        0, osg::Vec3(0.0f,0.0f,1000.0f), osg::Vec4(0.0f,0.0f,0.0f,0.0f) );

    osg::Node* light1 = effects::createLightSource(
        1, osg::Vec3(/*0.0f,-20.0f,0.0f*/0.0f,0.0f,1000.0f), osg::Vec4(1.5f,1.5f,1.5f,1.0f)
        );


    rootnode->getOrCreateStateSet()->setMode( GL_LIGHT0,
        osg::StateAttribute::ON );
    rootnode->getOrCreateStateSet()->setMode( GL_LIGHT1,
        osg::StateAttribute::ON );
    rootnode->addChild( light0 );
    rootnode->addChild( light1 );
}

class circleAimlessly : public osg::NodeCallback 
{
public:
    circleAimlessly(float angle=0.f): _angle(angle) {}
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        osg::MatrixTransform *tx = dynamic_cast<osg::MatrixTransform *>(node);
        if( tx != NULL)
        {
            _angle += osg::PI/180.0;
            tx->setMatrix( osg::Matrix::translate( 30.0, 0.0, 5.0) * 
                osg::Matrix::rotate( _angle, 0, 0, 1 ) );
        }
        traverse(node, nv);
    }
private:
    float _angle;
};


class MyGustCallback : public osg::NodeCallback
{

    public:

        MyGustCallback() {}

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            osgParticle::PrecipitationEffect* pe = dynamic_cast<osgParticle::PrecipitationEffect*>(node);
            
            float value = sin(nv->getFrameStamp()->getSimulationTime());
            if (value<-0.5)
            {
                pe->snow(1.0);
            }
            else
            {
                pe->rain(0.5);
            }
        
            traverse(node, nv);
        }
};

class EphemerisDataUpdateCallback : public osgEphemeris::EphemerisUpdateCallback
{
public:
    EphemerisDataUpdateCallback(): EphemerisUpdateCallback( "EphemerisDataUpdateCallback" )
    {}

    void operator()( osgEphemeris::EphemerisData *data )
    {
        time_t seconds = time(0L);
        struct tm *_tm = localtime(&seconds);

        data->dateTime.setYear( _tm->tm_year + 1900 ); // DateTime uses _actual_ year (not since 1900)
        data->dateTime.setMonth( _tm->tm_mon + 1 ); // DateTime numbers months from 1 to 12, not 0 to 11
        data->dateTime.setDayOfMonth( _tm->tm_mday + 1 ); // DateTime numbers days from 1 to 31, not 0 to 30
        data->dateTime.setHour( _tm->tm_hour );
        data->dateTime.setMinute( _tm->tm_min );
        data->dateTime.setSecond( _tm->tm_sec );
    }
};

// This handler lets you control the passage of time using keys.
// TODO: add keys to make the time increment by a given increment each frame
// with control over the increment.
class TimeChangeHandler : public osgGA::GUIEventHandler
{
public:
    TimeChangeHandler(osgEphemeris::EphemerisModel *ephem) : m_ephem(ephem) {}

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        if (!ea.getHandled() && ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN)
        {
            if (ea.getKey() == osgGA::GUIEventAdapter::KEY_KP_Add)
            {
                // Increment time
                // Hopefully the DateTime will wrap around correctly if we get 
                // to invalid dates / times...
                osgEphemeris::EphemerisData* data = m_ephem->getEphemerisData();
                if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT)          // Increment by one hour
                    data->dateTime.setHour( data->dateTime.getHour() + 1 );
                else if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_ALT)       // Increment by one day
                    data->dateTime.setDayOfMonth( data->dateTime.getDayOfMonth() + 1 );
                else if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL)      // Increment by one month
                    data->dateTime.setMonth( data->dateTime.getMonth() + 1 );
                else                                                                    // Increment by one minute
                    data->dateTime.setMinute( data->dateTime.getMinute() + 1 );

                return true;
            }

            else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_KP_Subtract)
            {
                // Decrement time
                // Hopefully the DateTime will wrap around correctly if we get 
                // to invalid dates / times...
                osgEphemeris::EphemerisData* data = m_ephem->getEphemerisData();
                if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT)          // Decrement by one hour
                    data->dateTime.setHour( data->dateTime.getHour() - 1 );
                else if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_ALT)       // Decrement by one day
                    data->dateTime.setDayOfMonth( data->dateTime.getDayOfMonth() - 1 );
                else if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL)      // Decrement by one month
                    data->dateTime.setMonth( data->dateTime.getMonth() - 1 );
                else                                                                    // Decrement by one minute
                    data->dateTime.setMinute( data->dateTime.getMinute() - 1 );

                return true;
            }

        }

        return false;
    }

    virtual void getUsage(osg::ApplicationUsage& usage) const
    {
        usage.addKeyboardMouseBinding("Keypad +",       "Increment time by one minute");
        usage.addKeyboardMouseBinding("Shift Keypad +", "Increment time by one hour"  );
        usage.addKeyboardMouseBinding("Alt Keypad +",   "Increment time by one day"   );
        usage.addKeyboardMouseBinding("Ctrl Keypad +",  "Increment time by one month" );
        usage.addKeyboardMouseBinding("Keypad -",       "Decrement time by one minute");
        usage.addKeyboardMouseBinding("Shift Keypad -", "Decrement time by one hour"  );
        usage.addKeyboardMouseBinding("Alt Keypad -",   "Decrement time by one day"   );
        usage.addKeyboardMouseBinding("Ctrl Keypad -",  "Decrement time by one month" );
    }

    osg::ref_ptr<osgEphemeris::EphemerisModel> m_ephem;
};


int main_scene( int argc, char** argv )
{
    osg::ArgumentParser arguments(&argc,argv);



#if 0
    // create the window to draw to.
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->x = 0;
    traits->y = 0;
    traits->width = 1920;
    traits->height = 1080;
    traits->windowDecoration = true;
    traits->doubleBuffer = true;
    traits->sharedContext = 0;

    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    osgViewer::GraphicsWindow* gw = dynamic_cast<osgViewer::GraphicsWindow*>(gc.get());
    if (!gw)
    {
        osg::notify(osg::NOTICE)<<"Error: unable to create graphics window."<<std::endl;
        return 1;
    }
#endif

	osg::DisplaySettings::instance()->setNumMultiSamples( 8 );

    osgViewer::Viewer viewer(arguments);
    arguments.reportRemainingOptionsAsUnrecognized();

	    // Set the clear color to black
    viewer.getCamera()->setClearColor(osg::Vec4(0,0,0,1));

#if 0
    viewer.getCamera()->setGraphicsContext(gc.get());
    viewer.getCamera()->setViewport(0,0,1920,1080);
#endif

    // tilt the scene so the default eye position is looking down on the model.
    osg::ref_ptr<osg::MatrixTransform> rootnode = new osg::MatrixTransform;
    //rootnode->setMatrix(osg::Matrix::rotate(osg::inDegrees(-90.0f),1.0f,0.0f,0.0f));

    // Use a default camera manipulator
    osgGA::TrackballManipulator* manip = new osgGA::TrackballManipulator;
    viewer.setCameraManipulator(manip);
    // Initially, place the TrackballManipulator so it's in the center of the scene
    manip->setHomePosition(osg::Vec3(0,1000,1000), osg::Vec3(0,1,0), osg::Vec3(0,0,1));
    manip->home(0);

    // Add some useful handlers to see stats, wireframe and onscreen help
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(rootnode->getOrCreateStateSet()));
    viewer.addEventHandler(new osgViewer::HelpHandler);

    // any option left unread are converted into errors to write out later.


    // report any errors if they have occurred when parsing the program arguments.
    if (arguments.errors())
    {
        arguments.writeErrorMessages(std::cout);
        return 1;
    }

    std::string animationName("Default");
    
    const osg::Quat quat0(0,          osg::X_AXIS,                      
                          0,          osg::Y_AXIS,
                          0,          osg::Z_AXIS ); 

    // auto model = osgDB::readNodeFile("an_124.dae");  /*a_319.3ds*/
    bool overlay = false;
    osgSim::OverlayNode::OverlayTechnique technique = osgSim::OverlayNode::VIEW_DEPENDENT_WITH_PERSPECTIVE_OVERLAY;//  osgSim::OverlayNode::OBJECT_DEPENDENT_WITH_ORTHOGRAPHIC_OVERLAY;
    //while (arguments.read("--object")) { technique = osgSim::OverlayNode::OBJECT_DEPENDENT_WITH_ORTHOGRAPHIC_OVERLAY; overlay=true; }
    //while (arguments.read("--ortho") || arguments.read("--orthographic")) { technique = osgSim::OverlayNode::VIEW_DEPENDENT_WITH_ORTHOGRAPHIC_OVERLAY; overlay=true; }
    //while (arguments.read("--persp") || arguments.read("--perspective")) { technique = osgSim::OverlayNode::VIEW_DEPENDENT_WITH_PERSPECTIVE_OVERLAY; overlay=true; }


    // load the nodes from the commandline arguments.
    auto model_parts  = creators::createModel(overlay, technique);
    
    osg::ref_ptr<osg::MatrixTransform> turn_node = new osg::MatrixTransform;
    //turn_node->setMatrix(osg::Matrix::rotate(osg::inDegrees(-90.0f),1.0f,0.0f,0.0f));
    turn_node->addChild(model_parts[0]);
    osg::Node* model = turn_node;//model_parts[0];

    if(model == nullptr)
    {
         osg::notify(osg::WARN) << "Can't load " <<  "an_124.dae";
        return 1;
    }
    else
    {
        osg::BoundingSphere loaded_bs = model->getBound();

        // create a bounding box, which we'll use to size the room.
        osg::BoundingBox bb;
        bb.expandBy(loaded_bs);

 #ifdef TEST_SKYBOX
        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable( new osg::ShapeDrawable(
            new osg::Sphere(osg::Vec3(), model->getBound().radius())) );
        geode->setCullingActive( false );


        osg::ref_ptr<SkyBox> skybox = new SkyBox;
        skybox->getOrCreateStateSet()->setTextureAttributeAndModes( 0, new osg::TexGen );
        skybox->setEnvironmentMap( 0,
            osgDB::readImageFile("Cubemap_snow/posx.jpg"), osgDB::readImageFile("Cubemap_snow/negx.jpg"),
            osgDB::readImageFile("Cubemap_snow/posy.jpg"), osgDB::readImageFile("Cubemap_snow/negy.jpg"),
            osgDB::readImageFile("Cubemap_snow/posz.jpg"), osgDB::readImageFile("Cubemap_snow/negz.jpg") );
        skybox->addChild( geode.get() );
#endif


        rootnode->addChild(model);

#ifdef TEST_SKYBOX
        rootnode->addChild( skybox.get() );
#endif
        //auto _skyStarField = new svSky::StarField();

        //rootnode->addChild(_skyStarField);
        //_skyStarField->setIlluminationFog(10.0f/*_illumination*/, 0.3f/*_fogDensity*/);


        {
            // create outline effect
            osg::ref_ptr<osgFX::Outline> outline = new osgFX::Outline;
            model_parts[1]->asGroup()->addChild(outline.get());

            outline->setWidth(4);
            outline->setColor(osg::Vec4(1,1,0,1));
            outline->addChild(model_parts[3]);
        }

#ifdef  TEST_EPHEMERIS
        osg::BoundingSphere bs = model->getBound();
        osg::ref_ptr<osgEphemeris::EphemerisModel> ephemerisModel = new osgEphemeris::EphemerisModel;

		ephemerisModel->setSkyDomeMirrorSouthernHemisphere(false);
		ephemerisModel->setSkyDomeUseSouthernHemisphere(false);

        // Optionally, Set the AutoDate and Time to false so we can control it with the GUI
        ephemerisModel->setAutoDateTime( false );

        // Optionally, uncomment this if you want to move the Skydome, Moon, Planets and StarField with the mouse
        // ephemerisModel->setMoveWithEyePoint(false);
        // ephemerisModel->setEphemerisUpdateCallback( new EphemerisDataUpdateCallback );

        rootnode->addChild( ephemerisModel.get() );

        // Set some acceptable defaults.
        double latitude = 43.4444;                                  // Adler, RF
        double longitude = 39.9469;
        osgEphemeris::DateTime dateTime( 2010, 4, 1, 8, 0, 0 );     // 8 AM
        double radius = 10000;                                      // Default radius in case no files were loaded above
        osg::BoundingSphere bs_root = rootnode->getBound();
        if (bs_root.valid())                                             // If the bs is not valid then the radius is -1
            radius = bs_root.radius()*2;                                 // which would result in an inside-out skydome...

        ephemerisModel->setLatitudeLongitude( latitude, longitude );
        ephemerisModel->setDateTime( dateTime );
        ephemerisModel->setSkyDomeRadius( radius );
		ephemerisModel->setMoveWithEyePoint(false);
#endif  // TEST_EPHEMERIS

        //AddLight(rootnode);

        // run optimization over the scene graph
        //osgUtil::Optimizer optimzer;
        //optimzer.optimize(rootnode);

        // set the scene to render
        viewer.setSceneData(rootnode);

        // must clear stencil buffer...
        unsigned int clearMask = viewer.getCamera()->getClearMask();
        viewer.getCamera()->setClearMask(clearMask | GL_STENCIL_BUFFER_BIT);
        viewer.getCamera()->setClearStencil(0);

#ifdef ENABLE_CUSTOM_ANIMATION
        std::list<std::string>  l; 
        auto root_group = model->asGroup();
        if(root_group)
        for(unsigned int i=0; i< root_group->getNumChildren(); ++i)
        {
            std::cout << "Child nodes: " << root_group->getChild(i)->getName() << std::endl;
            l.push_back( root_group->getChild(i)->getName());
            if(l.back() == "Shassis_LO")
            {
                root_group->getChild(i)->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON); 
                root_group->getChild(i)->getStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
                root_group->getChild(i)->getStateSet()->setRenderBinDetails(1, "DepthSortedBin"); 
                auto shassy = root_group->getChild(i)->asGroup();
                for(unsigned j=0; j< shassy->getNumChildren(); ++j)
                {
                     shassy->getChild(j)->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON); 
                     shassy->getChild(j)->getStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
                     shassy->getChild(j)->getStateSet()->setRenderBinDetails(1, "DepthSortedBin"); 

                     std::string s_name = shassy->getChild(j)->getName();
                     if (s_name=="lg_left" || s_name=="lg_right" || s_name=="lg_forward" || s_name=="lp18" || s_name=="lp21")
                     {
                         int sign = (s_name=="lg_left" || s_name=="lp21")?1:(s_name=="lg_right" || s_name=="lp18")?-1:1;
                         auto mt_ = shassy->getChild(j)->asTransform()->asMatrixTransform();
                         //auto pos = shassy->getChild(j)->asTransform()->asPositionAttitudeTransform();->getPosition()
                         {
                             osg::Quat quat;
                             if(s_name!="lg_forward")
                             {
                                     quat = osg::Quat (0,   osg::X_AXIS,
                                                       0,   osg::Y_AXIS,
                                 sign*(0.8 * osg::PI_2 ),   osg::Z_AXIS );
                             }
                             else
                             {
                                      quat = osg::Quat (osg::PI_2*1.5,   osg::X_AXIS,
                                                                    0,   osg::Y_AXIS,
                                                                    0,   osg::Z_AXIS );
                                
                             }
                             
                             auto bound = shassy->getChild(j)->getBound();
                             // set up the animation path
                             osg::AnimationPath* animationPath = new osg::AnimationPath;
                             animationPath->insert(0.0,osg::AnimationPath::ControlPoint(bound.center()+ osg::Vec3d(0,bound.radius()/2,0)/* *computeTargetToWorldMatrix(shassy->getChild(j))*/,quat));
                             animationPath->insert(1.0,osg::AnimationPath::ControlPoint(bound.center()+ osg::Vec3d(0,bound.radius()/2,0)/* *computeTargetToWorldMatrix(shassy->getChild(j))*/,quat0));
                             animationPath->insert(0.0,osg::AnimationPath::ControlPoint(bound.center() + osg::Vec3d(0,bound.radius()/2,0)/* *computeTargetToWorldMatrix(shassy->getChild(j))*/,quat));

                             //animationPath->insert(1.0,osg::AnimationPath::ControlPoint(osg::Vec3d(1.,1.,1.)*mt_->getMatrix(),quat0));
                             //animationPath->insert(0.0,osg::AnimationPath::ControlPoint(osg::Vec3d(1.,1.,1.)*mt_->getMatrix(),quat));
                         
                             animationPath->setLoopMode(osg::AnimationPath::SWING);

                             mt_->setUpdateCallback(new osg::AnimationPathCallback(animationPath));
                         }


                         auto mat_ =  mt_->getMatrix();

                         //int sign = (s_name=="lp2")?1:(s_name=="lp3")?-1:0; 
                         //osg::Matrix zRot;
                         //zRot.makeRotate(sign*osg::PI_4, 0.0,0.0,1.0);
                                          
                         //mt_->setMatrix( zRot*mat_ );
                     

                         auto shassy_l = shassy->getChild(j)->asGroup();
                         for(unsigned k=0; k< shassy_l->getNumChildren(); ++k)
                         {
                             shassy_l->getChild(k)->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON); 
                             shassy_l->getChild(k)->getStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
                             shassy_l->getChild(k)->getStateSet()->setRenderBinDetails(1, "DepthSortedBin"); 
                         
                         }
                     }
                }


            }

        }
#endif


        osg::ref_ptr<osgGA::StateSetManipulator> statesetManipulator = new osgGA::StateSetManipulator(viewer.getCamera()->getStateSet());
        viewer.addEventHandler(statesetManipulator.get());

        findNodeVisitor findNode("airplane"); 
        model->accept(findNode);

        auto node =  findNode.getFirst();
        if(node)
            viewer.addEventHandler(new AnimationHandler(/*node*/model_parts[1],animationName
                   ,[&](){effects::insertParticle(model->asGroup(),/*node*/model_parts[2]/*->asGroup()*/,osg::Vec3(00.f,00.f,00.f),0.f);}
                   ,[&](bool off){model_parts[2]->setUpdateCallback(off?nullptr:new circleAimlessly());}
                   ,[&](bool low){model_parts[4]->setNodeMask(low?0:0xffffffff);model_parts[5]->setNodeMask(low?0xffffffff:0);}
            ));

#ifdef  TEST_EPHEMERIS
        viewer.addEventHandler( new TimeChangeHandler( ephemerisModel.get() ) );
#endif

        //model_parts[2]->setNodeMask(/*0xffffffff*/0);           // ������ ���� ���������
        //model_parts[2]->setUpdateCallback(new circleAimlessly()); // ���� model_parts[2] ������� ���������� ����� ����� ���������� ������
		
		//Experiment with setting the LightModel to very dark to get better sun lighting effects
		{
			osg::ref_ptr<osg::StateSet> sset = rootnode->getOrCreateStateSet();
			osg::ref_ptr<osg::LightModel> lightModel = new osg::LightModel;
			lightModel->setAmbientIntensity( osg::Vec4( 0.0025, 0.0025,0.0025, 1.0 ));
			sset->setAttributeAndModes( lightModel.get() );
		}

		// create the light    
		//osg::LightSource* lightSource = new osg::LightSource;
		//model->asGroup()->addChild(lightSource);

		//osg::Light* light = lightSource->getLight();
		//light->setLightNum(0);
		//light->setPosition(osg::Vec4(0.0f,0.0f,1.0f,0.0f)); // directional light from above
		//light->setAmbient(osg::Vec4(0.8f,0.8f,0.8f,1.0f));
		//light->setDiffuse(osg::Vec4(0.2f,0.2f,0.2f,1.0f));
		//light->setSpecular(osg::Vec4(0.2f,0.2f,0.2f,1.0f));

#ifdef TEST_PRECIP   // ����� � ��� ����� ���� 
	     osg::ref_ptr<osgParticle::PrecipitationEffect> precipitationEffect = new osgParticle::PrecipitationEffect;		
         model->asGroup()->addChild(precipitationEffect.get());
		 precipitationEffect->snow(0.3);
#endif
		// Useless
		//rootnode->getOrCreateStateSet()->setMode( GL_LINE_SMOOTH, osg::StateAttribute::ON );
		//rootnode->getOrCreateStateSet()->setMode( GL_POLYGON_SMOOTH, osg::StateAttribute::ON );

#if TEST_NODE_TRACKER  // �������� ������ ��� ������
		osg::ref_ptr<osgGA::NodeTrackerManipulator> manip
			= new osgGA::NodeTrackerManipulator;
		 manip->setTrackNode(model_parts[1]/*.get()*/);
		 manip->setTrackerMode(osgGA::NodeTrackerManipulator::NODE_CENTER);
		 viewer.setCameraManipulator(manip.get());

#endif

#ifdef  PRINT_STRUCTURE
        InfoVisitor infoVisitor;
        model->accept( infoVisitor );
#endif

#if 0
        const GLubyte *renderer = glGetString( GL_RENDERER );
        const GLubyte *vendor = glGetString( GL_VENDOR );
        const GLubyte *version = glGetString( GL_VERSION );
        const GLubyte *glslVersion =
            glGetString( GL_SHADING_LANGUAGE_VERSION );
        GLint major, minor;
        //glGetIntegerv(GL_MAJOR_VERSION, &major);
        //glGetIntegerv(GL_MINOR_VERSION, &minor);
        printf("GL Vendor : %s\n", vendor);
        printf("GL Renderer : %s\n", renderer);
        printf("GL Version (string) : %s\n", version);
        printf("GL Version (integer) : %d.%d\n", major, minor);
        printf("GLSL Version : %s\n", glslVersion);	
#endif

        return viewer.run();
    }

    return 1;
}