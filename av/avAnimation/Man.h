#pragma once

#include <avDynamicObject\DynamicObject.h>

namespace avAnimation
{

struct IMan
{
    virtual void setAnimation ( unsigned char const *, size_t ) = 0;
    virtual void setSubobjectCulling ( const char* Name, bool CullingIsOn ) = 0;

    virtual void placeTool ( unsigned uAttachPositionID, unsigned uToolID ) = 0;
    virtual void clearAllTools ( ) = 0;
};

struct Man
    : public avDynamicObject::VisualDynamicObject
    , public IMan
{
    Man ( avCore::uint32 nID )
        : avDynamicObject::VisualDynamicObject ( nID )
    {
    }
};

Man * CreateMan ( avCore::uint32 nID, avCore::XML::XmlNode pInfoXMLRootElement );

} // end namespace

