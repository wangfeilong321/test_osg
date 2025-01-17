#include "stdafx.h"

#include "av/precompiled.h"

#include "Ephemeris.h" 

#include "av/FogLayer.h"
#include "av/CloudLayer.h"
#include "av/LightningLayer.h"
#include "av/EnvRenderer.h"


#include <osgEphemeris/EphemerisModel.h>  

#include "utils/callbacks.h"

namespace avSky
{
    struct Ephemeris::data : public osg::Referenced
    {    

        data():Referenced() {}
        osg::ref_ptr<FogLayer>                     _fogLayer;
        osg::ref_ptr<CloudsLayer>                  _cloudsLayer;
		osg::ref_ptr<LightningLayer>               _lightningLayer;
		
        osg::ref_ptr<osgEphemeris::EphemerisModel> _ephemerisModel;
        osg::ref_ptr<EphemerisDataUpdateCallback>  _eCallback;
    };

    class EphemerisDataUpdateCallback : virtual public osgEphemeris::EphemerisUpdateCallback
    {
    public:
        EphemerisDataUpdateCallback(Ephemeris *ephem) 
            : EphemerisUpdateCallback( "EphemerisDataUpdateCallback" )
            , _ephem      (ephem)
            , _handler    (new handler(_ephem.get()))
        {}

        void operator()( osgEphemeris::EphemerisData *data )
        {
            const osg::Light* sls = _ephem->_d->_ephemerisModel->getSunLightSource()->getLight();

            osg::ref_ptr<FogLayer>    _fogLayer  = _ephem->_d->_fogLayer;
            osg::ref_ptr<CloudsLayer> _skyClouds = _ephem->_d->_cloudsLayer;

            if(!_fogLayer || !_skyClouds)
                return;

            // Sun color and altitude little bit dummy 


            osg::Vec4 lightpos = sls->getPosition();
            osg::Vec3 lightDir = sls->getDirection();

            auto _sunAltitude = data->data[0].alt;
            auto _sunColor    = sls->getDiffuse(); 
            // try to calculate some diffuse term (base luminance level for diffuse term is [0, 0.9])
            static const float g_fDiffuseMax = 0.9f;

            osg::Vec3 _globalDiffuse = osg::Vec3(_sunColor.x(),_sunColor.y(),_sunColor.z());// _sunColor;
            // let's additionally fade diffuse when sun is absent
            const float
                fBelowHorizonFactorR = pow(cg::clamp(-2.0f, 8.0f, 0.f, 1.f)(_sunAltitude), 1.25f),
                fBelowHorizonFactorG = powf(fBelowHorizonFactorR, 1.2f),
                fBelowHorizonFactorB = powf(fBelowHorizonFactorR, 1.5f);
            _globalDiffuse = osg::componentMultiply(_globalDiffuse,osg::Vec3(fBelowHorizonFactorR,fBelowHorizonFactorG,fBelowHorizonFactorB));
            //_globalDiffuse.g *= fBelowHorizonFactorG;
            //_globalDiffuse.b *= fBelowHorizonFactorB;
            _globalDiffuse *= g_fDiffuseMax;

            ///////////////////////////////////////////////////////////////////////////////////////////////
            //       � ������� �����
            ///////////////////////////////////////////////////////////////////////////////////////////////
            // ambient-diffuse when fog or overcast - increase ambient, decrease diffuse
            const float fDiffuseOvercast = std::max(_fogLayer->getFogDensity()*_fogLayer->getFogDensity(), _skyClouds->getOvercastCoef());
            auto cFogDifCut = sls->getDiffuse()* (0.55f * fDiffuseOvercast);//osg::Vec4f(_globalDiffuse * (0.55f * fDiffuseOvercast),1.0f);
            // decrease diffuse with fog
            auto cFogDif = sls->getDiffuse() - cFogDifCut;
            // increase ambient with fog
            auto cFogAmb = sls->getAmbient() +  cFogDifCut * 0.5f;
            // dim specular
            const float fSpecularOvercastCoef = pow(1.f - fDiffuseOvercast, 0.5f);
            auto cFogSpec = sls->getSpecular() * fSpecularOvercastCoef;

            // recalc illumination based on new foggy values
            const float fIllumDiffuseFactor = 1.f - _skyClouds->getOvercastCoef();
            auto illumination = cg::luminance_crt(cFogAmb + cFogDif * fIllumDiffuseFactor);
            
            _ephem->setIllumination(illumination);
            // FIXME ���� ���������� � ��������� ���������������� �������� ��������� 

            // when ambient is low - get it's color directly (to make more realistic fog at dusk/dawn)
            // also when overcast - modulate color with ambient
            const float fFogDesatFactor = cg::bound(cFogAmb.x() * 1.5f, 0.f, 1.f);
            //auto fog_color = cg::lerp01(fFogDesatFactor,/*cFogAmb*/cFogDif/*osg::Vec4f(_globalDiffuse,1.0)*/, osg::Vec4f(illumination,illumination,illumination,1.0) );
            auto fog_color = cg::lerp01(/*cFogAmb*/cFogDif/*osg::Vec4f(_globalDiffuse,1.0)*/, osg::Vec4f(illumination,illumination,illumination,1.0),fFogDesatFactor );

            // save fog
            _fogLayer->setFogParams( osg::Vec3(fog_color.x(),fog_color.y(),fog_color.z()), _fogLayer->getFogDensity());
            //data_.fog_exp_coef = _fogLayer->getFogExp2Coef();

            const float fCloudLum = cg::max(0.06f, illumination);
            _skyClouds->setCloudsColors(osg::Vec3f(fCloudLum,fCloudLum,fCloudLum), osg::Vec3f(fCloudLum,fCloudLum,fCloudLum));

            _skyClouds->setRotationSiderealTime(-float(fmod(data->localSiderealTime / 24.0, 1.0)) * 360.0f);
             
            
             osg::Vec4 diffuse =  cFogDif*1.8f;     //sls->getDiffuse();
             osg::Vec4 ambient =  cFogAmb*1.2;      //sls->getAmbient();
             osg::Vec4 specular = cFogSpec*1.8;     // sls->getSpecular();
             
             ambient.w() = illumination;
             specular.w() = avCore::GetEnvironment()->GetWeatherParameters().RainDensity;                    

            _ephem->_specularUniform->set(specular);
            _ephem->_ambientUniform->set(ambient);
            _ephem->_diffuseUniform->set(diffuse);
            
          
            _ephem->_lightDirUniform->set( lightpos * _ephem->getModelViewMatrix() /*osg::Vec4(lightDir,1.)*/);

                    
       }

        osgGA::GUIEventHandler* getHandler() {return _handler.get()/*.release()*/;};

    private:
        class handler : public osgGA::GUIEventHandler
        {

        public:  
            handler(Ephemeris *ephem) 
                : _ephem(ephem) 
                , _currCloud  (avSky::cirrus)
                , _intensity(0.1)
            {}

            virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
            {

                auto _fogLayer =   _ephem->_d->_fogLayer;
                auto _skyClouds  = _ephem->_d->_cloudsLayer;
				auto _skyLightning  = _ephem->_d->_lightningLayer;

                auto ephem       = _ephem->_d->_ephemerisModel;
                const osg::Vec3f _color = _fogLayer->getFogColor();    
                
                if (!ea.getHandled() && ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN)
                {
                    if (ea.getKey() == osgGA::GUIEventAdapter::KEY_KP_Insert)
                    {
                        if(_skyClouds)
                        {
                            int cc = _currCloud;cc++;
                            _currCloud = static_cast<avSky::cloud_type>(cc);
                            if(_currCloud >= avSky::clouds_types_num)
                                _currCloud = avSky::none;

                            _skyClouds->setCloudsTexture(_currCloud);
                        }
                        return true;
                    }
                    else
                    if (ea.getKey() == osgGA::GUIEventAdapter::KEY_KP_Page_Up)
                    {
                        if(_skyClouds) _skyClouds->setCloudsDensity(cg::bound(_skyClouds->getCloudsDensity() +.1f,.0f,1.0f));

                        return true;
                    }
                    else
                    if (ea.getKey() == osgGA::GUIEventAdapter::KEY_KP_Page_Down)
                    {
                        if(_skyClouds) _skyClouds->setCloudsDensity(cg::bound(_skyClouds->getCloudsDensity() -.1f,.0f,1.0f));

                        return true;
                    }
                    else
                    if (ea.getKey() == osgGA::GUIEventAdapter::KEY_KP_Add)
                    {
                        // Increment time
                        // Hopefully the DateTime will wrap around correctly if we get 
                        // to invalid dates / times...

                        // ����������� ������ �� ����� ����� ������  
                        osgEphemeris::DateTime dt = ephem->getDateTime();

                        if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT)          // Increment by one hour
                            dt.setHour( dt.getHour() + 1 );
                        else if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_ALT)       // Increment by one day
                            dt.setDayOfMonth( dt.getDayOfMonth() + 1 );
                        else if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL)      // Increment by one month
                            dt.setMonth( dt.getMonth() + 1 );
                        else                                                                    // Increment by one minute
                            dt.setMinute( dt.getMinute() + 1 );

                        ephem->setDateTime(dt);
                        //std::cout << "**** Sun light ****" << std::endl;
                        //auto sls = m_ephem->getSunLightSource()->getLight();

                        //std::cout << "Ambient: " << sls->getAmbient() << std::endl;
                        //std::cout << "Diffuse: " << sls->getDiffuse() << std::endl;
                        //std::cout << "Specular: " << sls->getSpecular() << std::endl;

                        //std::cout << "********************" << std::endl;
                        //
                        //osg::Vec4 a   = sls->getAmbient();
                        //osg::Vec4 d   = sls->getDiffuse();
                        //a[3] = 0;//(d[0] + d[1] + d[2])/3.0;
                        //sls->setAmbient(a);

                        return true;
                    }

                    else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_KP_Subtract)
                    {
                        // Decrement time
                        osgEphemeris::DateTime dt = ephem->getDateTime();
                        if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT)          // Decrement by one hour
                            dt.setHour( dt.getHour() - 1 );
                        else if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_ALT)       // Decrement by one day
                            dt.setDayOfMonth( dt.getDayOfMonth() - 1 );
                        else if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL)      // Decrement by one month
                            dt.setMonth( dt.getMonth() - 1 );
                        else                                                                    // Decrement by one minute
                            dt.setMinute( dt.getMinute() - 1 );

                        ephem->setDateTime(dt);

                        return true;
                    }
                    else
                    if (ea.getKey() == osgGA::GUIEventAdapter::KEY_I)
                    {

                        osgEphemeris::EphemerisData* data = ephem->getEphemerisData();

                       data->turbidity += 1 ;
                        
                        return true;
                    }
                    else
                    if (ea.getKey() == osgGA::GUIEventAdapter::KEY_O)
                    {
                        osgEphemeris::EphemerisData* data = ephem->getEphemerisData();
                        data->turbidity -= 1 ;

                        return true;
                    }
                    else
                    if (ea.getKey()==osgGA::GUIEventAdapter::KEY_Rightbracket )
                    { 
                        _intensity += 0.1;
                        if(_intensity > 1.0)
                            _intensity = 1.0;
                        else
                            _fogLayer->setFogParams(_color,_intensity);

                        return true;
                    } else
                    if (ea.getKey()== osgGA::GUIEventAdapter::KEY_Leftbracket)
                    { 
                        _intensity -= 0.1;
                        if(_intensity < 0.0) 
                            _intensity = 0.0;
                        else
                           _fogLayer->setFogParams(_color,_intensity);

                        return true;
                    }
					else
					if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Page_Up)
					{
						avCore::GetEnvironment()->m_EnvironmentParameters.WindDirection += cg::point_3f(0,.5,0);
						return true;
					}
					else
					if (ea.getKey() == osgGA::GUIEventAdapter:: KEY_Page_Down)
					{
						avCore::GetEnvironment()->m_EnvironmentParameters.WindDirection -= cg::point_3f(0,.5,0);
						return true;
					}
                }
                return false;
            } 

        private:
            osg::ref_ptr<Ephemeris>                           _ephem;
            avSky::cloud_type                                 _currCloud;
            float                                             _intensity;
        };

    private:
        osg::ref_ptr<Ephemeris>                    _ephem;
        osg::ref_ptr<handler>                      _handler;

    };

        
    Ephemeris::Ephemeris(osg::Group * sceneRoot,osg::Group * terrainNode)
        : _d          (new data)
        , _sceneRoot  (sceneRoot)
        , _terrainNode(terrainNode)
    {
        Initialize();    
        
        osg::StateSet * pSceneSS = sceneRoot->getOrCreateStateSet();
        _ambientUniform = new osg::Uniform("ambient", osg::Vec4f(1.f, 1.f, 1.f, 0.f));
        pSceneSS->addUniform(_ambientUniform.get());

        _diffuseUniform = new osg::Uniform("diffuse", osg::Vec4f(1.f, 1.f, 1.f, 0.f));
        pSceneSS->addUniform(_diffuseUniform.get());

        _specularUniform = new osg::Uniform("specular", osg::Vec4f(1.f, 1.f, 1.f, 0.f));
        pSceneSS->addUniform(_specularUniform.get());
        
        _lightDirUniform = new osg::Uniform("light_vec_view", osg::Vec4f(0.f, 0.f, 0.f, 1.f));
        pSceneSS->addUniform(_lightDirUniform.get());

        // callbacks setup
        setCullCallback(utils::makeNodeCallback(this, &Ephemeris::cull, true));
    }

    bool Ephemeris::Initialize()
    {
        _d->_ephemerisModel = new osgEphemeris::EphemerisModel;
        _d->_ephemerisModel->setSkyDomeMirrorSouthernHemisphere(false);
        _d->_ephemerisModel->setAutoDateTime( false );

        // Set some acceptable defaults.
        double latitude = 43.4444;                                  // Adler, RF
        double longitude = 39.9469;
        
        double radius = 20000;                                      // Default radius in case no files were loaded above
        
        setSummerTime();

        osg::BoundingSphere bs = _terrainNode.valid()?_terrainNode->getBound():osg::BoundingSphere();
        if (bs.valid())                                             // If the bs is not valid then the radius is -1
            radius = bs.radius()*2;                                 // which would result in an inside-out skydome...

        _d->_ephemerisModel->setLatitudeLongitude( latitude, longitude );
        _d->_ephemerisModel->setSkyDomeRadius    ( radius );
        // Optionally, uncomment this if you want to move the Skydome, Moon, Planets and StarField with the mouse
        _d->_ephemerisModel->setMoveWithEyePoint (false);

        _d->_fogLayer = new FogLayer(_sceneRoot->asGroup());
        _d->_ephemerisModel->asGroup()->addChild(_d->_fogLayer.get());
        auto fogColor = osg::Vec3f(1.0,1.0,1.0);
        _d->_fogLayer->setFogParams(fogColor,0.1);    // (�������� � 0.1 �� �������� 1.0)
        float coeff = _d->_fogLayer->getFogExp2Coef();
       
        _d->_cloudsLayer = new avSky::CloudsLayer(_sceneRoot->asGroup());
        _d->_ephemerisModel->asGroup()->addChild(_d->_cloudsLayer.get());
        _d->_cloudsLayer->setCloudsColors( osg::Vec3f(1.0,1.0,1.0), osg::Vec3f(1.0,1.0,1.0) );
		
		_d->_lightningLayer = new avSky::LightningLayer(_sceneRoot->asGroup());
		_d->_ephemerisModel->asGroup()->addChild(_d->_lightningLayer.get());
		_d->_lightningLayer->setColors( osg::Vec3f(1.0,1.0,1.0), osg::Vec3f(1.0,1.0,1.0) );
        _d->_lightningLayer->setRotationSiderealTime(270);

        _d->_eCallback = new EphemerisDataUpdateCallback(this);
        _d->_ephemerisModel->setEphemerisUpdateCallback( _d->_eCallback );

#ifndef TEST_EVN_CUBE_MAP   // Environment to cube map renderer 
        osg::ref_ptr<osg::Group> fbo_node = new osg::Group;
        fbo_node->addChild(_d->_ephemerisModel.get());
        _sceneRoot->asGroup()->addChild(avEnv::createPrerender(fbo_node,osg::NodePath(),0,osg::Vec4(1.0f, 1.0f, 1.0f, 0.0f),osg::Camera::FRAME_BUFFER_OBJECT));
         
        setStarFieldMask(NODE_STARFIELD_MASK);
#else
        _sceneRoot->asGroup()->addChild(_d->_ephemerisModel.get());
#endif
        return true;
    }

    void Ephemeris::setTime()
    {
        time_t seconds = time(0L);
        struct tm *_tm = localtime(&seconds);
        osgEphemeris::DateTime dt;
        dt.setYear( _tm->tm_year + 1900 ); // DateTime uses _actual_ year (not since 1900)
        dt.setMonth( _tm->tm_mon + 1 ); // DateTime numbers months from 1 to 12, not 0 to 11
        dt.setDayOfMonth( _tm->tm_mday + 1 ); // DateTime numbers days from 1 to 31, not 0 to 30
        dt.setHour( _tm->tm_hour );
        dt.setMinute( _tm->tm_min );
        dt.setSecond( _tm->tm_sec );
        _d->_ephemerisModel->setDateTime( dt );
    }
    
    void Ephemeris::setSummerTime()
    {
        time_t seconds = time(0L);
        struct tm *_tm = localtime(&seconds);
        osgEphemeris::DateTime dt;
        dt.setYear( _tm->tm_year + 1900 ); // DateTime uses _actual_ year (not since 1900)
        dt.setMonth( 6 ); // DateTime numbers months from 1 to 12, not 0 to 11
        dt.setDayOfMonth( _tm->tm_mday + 1 ); // DateTime numbers days from 1 to 31, not 0 to 30
        dt.setHour( _tm->tm_hour );
        dt.setMinute( _tm->tm_min );
        dt.setSecond( _tm->tm_sec );
        _d->_ephemerisModel->setDateTime( dt );
    }

    void Ephemeris::setSkyDomeRadius(double radius)
    {
       _d->_ephemerisModel->setSkyDomeRadius    ( radius );
    }

    void Ephemeris::setSunLightSource(osg::LightSource* ls)
    {
        _d->_ephemerisModel->setSunLightSource(ls);
    }

    osg::LightSource* Ephemeris::getSunLightSource()
    {
        return _d->_ephemerisModel->getSunLightSource(); 
    }

    void Ephemeris::setStarFieldMask(osg::Node::NodeMask nm)
    {
        auto sf =  findFirstNode(_d->_ephemerisModel,"StarField",findNodeVisitor::not_exact);
        if(sf) 
            sf->setNodeMask(nm); 
    }


    osgGA::GUIEventHandler* Ephemeris::getEventHandler()
    {
        return  _d->_eCallback->getHandler();
    }

    // cull method
    void Ephemeris::cull( osg::NodeVisitor * pNV )
    {
        // get cull visitor
        osgUtil::CullVisitor * pCV = static_cast<osgUtil::CullVisitor *>(pNV);
        avAssert(pCV);
        
        _mv = *pCV->getModelViewMatrix();

        // cull down

        pNV->traverse(*this);

    }

    void Ephemeris::setIllumination(float illum)
    { 
        _illum = illum;
        avCore::GetEnvironment()->setIllumination(illum);   
    }


    //////////////////////////////////////////////////////////////////////////
    float Ephemeris::GetSunIntensity() const
    {
        return /*_cSkydomePtr->getIllumination()*/_illum;
    }

    //////////////////////////////////////////////////////////////////////////
    const osg::Vec3f & Ephemeris::GetFogColor() const
    {
        return /*_cSkydomePtr->getFogColor()*/_d->_fogLayer->getFogColor();
    }
}
