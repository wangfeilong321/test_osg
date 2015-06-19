#include "stdafx.h"

#include "Scene.h"
#include "Lights.h"

#include "LightManager.h"

#include "utils/callbacks.h"



namespace avScene
{


osg::ref_ptr<LightManager> LightManager::g_Instance;


enum LightManagerMessage
{
    CreateDynamicLight,
    DeleteDynamicLight,
    UpdateDynamicLight,
};

typedef std::map<std::string, LightManagerMessage> LightManagerMessageMap;
typedef LightManagerMessageMap::value_type LightManagerMessageMapValue;

static const LightManagerMessageMapValue g_MessageTable[] =
{
    LightManagerMessageMapValue("CreateDynamicLight", CreateDynamicLight),
    LightManagerMessageMapValue("DeleteDynamicLight", DeleteDynamicLight),
    LightManagerMessageMapValue("UpdateDynamicLight", UpdateDynamicLight),
};
static const unsigned g_nMessageTableSize = sizeof(g_MessageTable) / sizeof(g_MessageTable[0]);
static const LightManagerMessageMap g_MessageMap(&g_MessageTable[0], &g_MessageTable[0] + g_nMessageTableSize);




LightManager::LightManager()
{
    setDataVariance(DYNAMIC);
    setUpdateCallback(utils::makeNodeCallback(this, &LightManager::update));
}

LightManager::~LightManager()
{
}

void LightManager::Create()
{
    avAssert(!g_Instance.valid());
    g_Instance = new LightManager();
}

void LightManager::Release()
{
    avAssert(g_Instance.valid());
    g_Instance = NULL;
}

LightManager * LightManager::GetInstance()
{
    return g_Instance;
}


#if 0
void LightManager::OnEvent( const char * name, utils::MessageManager::MessageStream & stream )
{
    avAssert(name != NULL);

    const LightManagerMessageMap::const_iterator it = g_MessageMap.find(name);

    if (it == g_MessageMap.end())
    {
        svError("Unknown event");
        return;
    }

    const LightManagerMessage message = it->second;

    switch (message)
    {
    case CreateDynamicLight:
        {
            const unsigned lightID = stream.get<utils::uint32_t>();

            Light & light = m_LightsMap[lightID];

            const unsigned objectID = stream.get<utils::uint32_t>();
            svDynamicObject::VisualDynamicObject * dynamicObject = utils::ObjectManager::GetInstance()->GetObjectByID<svDynamicObject::VisualDynamicObject>(utils::ObjectBase::OBJECT_DYNAMIC, objectID);
            avAssert(dynamicObject != NULL);
            light.transform = dynamicObject->GetVisualModel();

            const float spotFalloff0 = osg::DegreesToRadians(stream.get<float>());
            const float spotFalloff1 = osg::DegreesToRadians(stream.get<float>());
            light.spotFalloff = cg::range_2f(spotFalloff0, spotFalloff1);

            const float distanceFalloff0 = stream.get<float>();
            const float distanceFalloff1 = stream.get<float>();
            light.distanceFalloff = cg::range_2f(distanceFalloff0, distanceFalloff1);

            light.color.r = stream.get<float>();
            light.color.g = stream.get<float>();
            light.color.b = stream.get<float>();
        }
        break;

    case DeleteDynamicLight:
        {
            const unsigned lightID = stream.get<utils::uint32_t>();

            LightsMap::const_iterator it = m_LightsMap.find(lightID);
            avAssert(it != m_LightsMap.end());

            m_LightsMap.erase(it);
        }
        break;

    case UpdateDynamicLight:
        {
            const unsigned lightID = stream.get<utils::uint32_t>();
            avAssert(m_LightsMap.find(lightID) != m_LightsMap.end());

            Light & light = m_LightsMap[lightID];

            light.position.x = stream.get<double>();
            light.position.y = stream.get<double>();
            light.position.z = stream.get<double>();
            std::swap(light.position.x, light.position.y);

            const float heading = osg::DegreesToRadians(stream.get<float>());
            const float pitch = osg::DegreesToRadians(stream.get<float>());
            light.direction = cg::vector_3(cos(pitch) * sin(heading), cos(pitch) * cos(heading), sin(pitch));

            light.active = stream.get<bool>();
        }
        break;

    default:
        svError("message not handled");
        break;
    }
}
#endif

void  LightManager::addLight(/*uint32_t id,*/ osg::MatrixTransform* mt )
{
    const unsigned lightID = m_LightsMap.size()/*id*/;
    
    Light & light = m_LightsMap[lightID];

    light.transform = mt;

    const float spotFalloff0 = osg::DegreesToRadians(15.f);
    const float spotFalloff1 = osg::DegreesToRadians(45.f);
    light.spotFalloff = cg::range_2f(spotFalloff0, spotFalloff1);

    const float distanceFalloff0 = 80.f;
    const float distanceFalloff1 = 220.f;
    light.distanceFalloff = cg::range_2f(distanceFalloff0, distanceFalloff1);

    light.color.r = 0.99;
    light.color.g = 0.99;
    light.color.b = 0.99;
    
    light.position = cg::point_3f(0,0,0);

    const float heading = osg::DegreesToRadians(0.f);
    const float pitch = osg::DegreesToRadians(0.f);
    light.direction = as_vector(cg::point_3f(cos(pitch) * sin(heading), cos(pitch) * cos(heading), sin(pitch) ));

    light.active = true;

}

void LightManager::addLight(/*uint32_t id,*/ const Light& data)
{
    const unsigned lightID = m_LightsMap.size()/*id*/;

    Light & light = m_LightsMap[lightID];

    light = data;
}

void LightManager::update( osg::NodeVisitor * nv )
{
    avScene::Scene * scene = GetScene();

    if (scene == NULL)
        return;

    avScene::Lights * lights = scene->getLights();

    if (lights == NULL)
        return;

    for (LightsMap::const_iterator it = m_LightsMap.cbegin(); it != m_LightsMap.cend(); ++it)
    {
        const Light & light = it->second;

        if (!light.active)
            continue;
      
#if 0
        osg::Matrix matrix = light.transform->getMatrix() * utils::GetCoordinateSystem()->GetLCS2LTPMatrix();
        matrix4d transform = matrix4d(matrix.ptr(), matrix::unscaled).transpose();
#else
        
        osg::Matrix matrix; 
        
        if(light.transform)
        if(light.transform->asMatrixTransform())
        {
           osg::Node* parent = light.transform->getParent(0);
           if(parent->asTransform())
                matrix =  light.transform->asMatrixTransform()->getMatrix() * parent->asTransform()->asMatrixTransform()->getMatrix();
           else
                matrix =  light.transform->asMatrixTransform()->getMatrix() ;
        }

        cg::matrix_4 trm = from_osg_matrix(matrix);
        cg::transform_4 transform = trm.transpose();
#endif
        const cg::point_3f position = transform.treat_point(light.position );
        const cg::vector_3 direction = transform.treat_vector(light.direction);
        
        cg::range_2f spotFalloff = light.spotFalloff;

#if 1
        //const cg::point_3f world_light_dir = mv_.treat_vector(spot.view_dir, false);
        //// angle corrected
        //auto corrected_spot = spot;
        //if (!corrected_spot.angle_falloff.empty())
        //    corrected_spot.angle_falloff |= cg::lerp01(spot.angle_falloff.hi(), 65.f, cg::bound(-world_light_dir.z, 0.f, 1.f));
        
        const cg::point_3f world_light_dir = transform.treat_vector(light.direction, false);
        // angle corrected

        if (!spotFalloff.empty())
            spotFalloff |= cg::lerp01(spotFalloff.hi(), 65.f, cg::bound(-world_light_dir.z, 0.f, 1.f));
#endif

        lights->AddLight(avScene::GlobalInfluence, avScene::ConicalLight,
            position, direction, light.distanceFalloff, spotFalloff, 
            light.color, 0.60f, 0.35f);
    }

}


} // namespace svScene
