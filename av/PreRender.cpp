#include "stdafx.h"
#include "Prerender.h"

#define GL_RGBA16F 0x881A
#define GL_RGBA32F 0x8814


Prerender::Prerender(int width, int height)
{
    init(width,height);
}

/** Copy constructor using CopyOp to manage deep vs shallow copy.*/
Prerender::Prerender(const Prerender& prerender,const osg::CopyOp& copyop):
    osg::CameraNode(prerender,copyop),
    _texture(prerender._texture)
{
}

void Prerender::traverse(osg::NodeVisitor& nv)
{
    // So do not allow any Visitors except CULL_VISITOR
 
    if (nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
         osg::Group::traverse(nv);
}

osg::Texture2D * Prerender::createTexture(int width, int height)
{
    osg::Texture2D* texture = new osg::Texture2D;
    texture->setTextureSize(width,height);
    texture->setInternalFormat(/*GL_RGBA*/GL_RGBA16F);
    texture->setSourceFormat(GL_RGBA);
    texture->setSourceType(GL_UNSIGNED_BYTE);
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
    texture->setFilter(osg::Texture::MIN_FILTER,osg::Texture::LINEAR_MIPMAP_LINEAR);
    texture->setFilter(osg::Texture::MAG_FILTER,osg::Texture::LINEAR);
    texture->setBorderColor(osg::Vec4d(0,0,0,0));
    // texture->setUseHardwareMipMapGeneration(false);
    texture->setNumMipmapLevels(4);
    
    return texture;
}

void Prerender::init(int width, int height)
{
    _texture = createTexture(width,height);

    setClearMask(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    //setClearColor(osg::Vec4(0.0,0.0,0.0,1.0));
    setClearDepth(1.0);
    
    setClearColor(osg::Vec4(0.0,0.0,0.0,0.0));
    setColorMask(true, true, true, true);

    // just inherit the main cameras view
    setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    setProjectionMatrix(osg::Matrixd::identity());
    setViewMatrix(osg::Matrixd::identity());

    // set viewport
    setViewport(0,0,width,height);

    // set the camera to render before the main camera.
    setRenderTargetImplementation(osg::CameraNode::FRAME_BUFFER_OBJECT);
    setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    setRenderOrder(osg::CameraNode::PRE_RENDER);

    // attach the texture and use it as the color buffer.
    attach(osg::CameraNode::COLOR_BUFFER, _texture.get(), 0, 0, true, 0, 0);

}

