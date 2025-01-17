#include "stdafx.h"
#ifndef Q_MOC_RUN
#include <SPK.h>
#include <SPK_GL.h>
#endif
#include "SparkDrawable.h"

#define GET_TEXTURE_ID( name, var ) \
    GLuint var = 0; itr = textureIDMap.find(name); \
    if ( itr!=textureIDMap.end() ) var = itr->second;

SPK::SPK_ID createFire( const SparkDrawable::TextureIDMap& textureIDMap, int screenWidth, int screenHeight, float scale_coeff )
{
    SparkDrawable::TextureIDMap::const_iterator itr;
    GET_TEXTURE_ID( "fire", textureFire );
    GET_TEXTURE_ID( "explosion", textureSmoke );
    
    // Renderers
    SPK::GL::GLQuadRenderer* fireRenderer = SPK::GL::GLQuadRenderer::create();
    fireRenderer->setScale(0.3f*scale_coeff,0.3f*scale_coeff);
    fireRenderer->setTexturingMode(SPK::TEXTURE_2D);
    fireRenderer->setTexture(textureFire);
    fireRenderer->setTextureBlending(GL_MODULATE);
    fireRenderer->setBlending(SPK::BLENDING_ADD);
    fireRenderer->enableRenderingHint(SPK::DEPTH_WRITE,false);
    fireRenderer->setAtlasDimensions(2,2);

    SPK::GL::GLQuadRenderer* smokeRenderer = SPK::GL::GLQuadRenderer::create();
    smokeRenderer->setScale(0.3f*scale_coeff,0.3f*scale_coeff);
    smokeRenderer->setTexturingMode(SPK::TEXTURE_2D);
    smokeRenderer->setTexture(textureSmoke);
    smokeRenderer->setTextureBlending(GL_MODULATE);
    smokeRenderer->setBlending(SPK::BLENDING_ALPHA);
    smokeRenderer->enableRenderingHint(SPK::DEPTH_WRITE,false);
    smokeRenderer->setAtlasDimensions(2,2);

    // Models
    SPK::Model* fireModel = SPK::Model::create(
        SPK::FLAG_RED | SPK::FLAG_GREEN | SPK::FLAG_BLUE | SPK::FLAG_ALPHA |
        SPK::FLAG_SIZE | SPK::FLAG_ANGLE | SPK::FLAG_TEXTURE_INDEX,
        SPK::FLAG_RED | SPK::FLAG_GREEN | SPK::FLAG_ALPHA | SPK::FLAG_ANGLE,
        SPK::FLAG_RED | SPK::FLAG_GREEN | SPK::FLAG_TEXTURE_INDEX | SPK::FLAG_ANGLE,
        SPK::FLAG_SIZE);
    fireModel->setParam(SPK::PARAM_RED,0.8f,0.9f,0.8f,0.9f);
    fireModel->setParam(SPK::PARAM_GREEN,0.5f,0.6f,0.5f,0.6f);
    fireModel->setParam(SPK::PARAM_BLUE,0.3f);
    fireModel->setParam(SPK::PARAM_ALPHA,0.4f,0.0f);
    fireModel->setParam(SPK::PARAM_ANGLE,0.0f,2.0f * osg::PI,0.0f,2.0f * osg::PI);
    fireModel->setParam(SPK::PARAM_TEXTURE_INDEX,0.0f,4.0f);
    fireModel->setLifeTime(1.0f,1.5f);

    SPK::Interpolator* interpolator = fireModel->getInterpolator(SPK::PARAM_SIZE);
    interpolator->addEntry(0.5f,2.0f,5.0f);
    interpolator->addEntry(1.0f,0.0f);


#if 1
    SPK::Model* smokeModel = SPK::Model::create(
        SPK::FLAG_RED | SPK::FLAG_GREEN | SPK::FLAG_BLUE | SPK::FLAG_ALPHA |
        SPK::FLAG_SIZE | SPK::FLAG_ANGLE | SPK::FLAG_TEXTURE_INDEX,
        SPK::FLAG_RED | SPK::FLAG_GREEN | SPK::FLAG_SIZE | SPK::FLAG_ANGLE,
        SPK::FLAG_TEXTURE_INDEX | SPK::FLAG_ANGLE,
        SPK::FLAG_ALPHA);
    
    smokeModel->setParam(SPK::PARAM_RED,0.3f,0.2f);
    smokeModel->setParam(SPK::PARAM_GREEN,0.25f,0.2f);
    smokeModel->setParam(SPK::PARAM_BLUE,0.2f);
    smokeModel->setParam(SPK::PARAM_ALPHA,0.2f,0.0f);
    smokeModel->setParam(SPK::PARAM_SIZE,5.0,15.0f);
    smokeModel->setParam(SPK::PARAM_TEXTURE_INDEX,0.0f,4.0f);
    // ���-�� ��� ���������� ��������
    //smokeModel->setParam(SPK::PARAM_ANGLE,0.0f,2.0f * osg::PI,0.0f,2.0f * osg::PI);
    FIXME(Tests)
    smokeModel->setLifeTime(1.0f,5.0f/*25.0f*/);
    
    interpolator = smokeModel->getInterpolator(SPK::PARAM_ALPHA);
    interpolator->addEntry(0.0f,0.0f);
    interpolator->addEntry(0.2f,0.2f);
    interpolator->addEntry(1.0f,0.0f);
#else
    // Creates the model
    SPK::Model* smokeModel = SPK::Model::create(
        SPK::FLAG_SIZE | SPK::FLAG_ALPHA | SPK::FLAG_TEXTURE_INDEX | SPK::FLAG_ANGLE,
        SPK::FLAG_SIZE | SPK::FLAG_ALPHA,
        SPK::FLAG_SIZE | SPK::FLAG_TEXTURE_INDEX | SPK::FLAG_ANGLE);

    smokeModel->setParam(SPK::PARAM_SIZE,5.0f,10.0f,100.0f,200.0f);
    smokeModel->setParam(SPK::PARAM_ALPHA,1.0f,0.0f);
    smokeModel->setParam(SPK::PARAM_TEXTURE_INDEX,0.0f,4.0f);
    smokeModel->setParam(SPK::PARAM_ANGLE,0.0f,osg::PI * 0.5f,0.0f,1.0f * osg::PI);
    smokeModel->setLifeTime(2.0f,5.0f);
#endif

    FIXME(Tests)
    const float base_z = -1.f;
    // Emitters
    // The emitters are arranged so that the fire looks realistic
    SPK::StraightEmitter* fireEmitter1 = SPK::StraightEmitter::create(SPK::Vector3D(0.0f,0.0f,1.0f));
    fireEmitter1->setZone(SPK::Sphere::create(SPK::Vector3D(0.0f,0.0f,/*-1.0f*/base_z),0.5f * scale_coeff));
    fireEmitter1->setFlow(40 *scale_coeff);
    fireEmitter1->setForce(1.0f,2.5f);

    SPK::StraightEmitter* fireEmitter2 = SPK::StraightEmitter::create(SPK::Vector3D(1.0f,0.0f,0.6f));
    fireEmitter2->setZone(SPK::Sphere::create(SPK::Vector3D(0.15f,0.075f,/*-1.2f*/base_z - .2f),0.1f * scale_coeff));
    fireEmitter2->setFlow(15*scale_coeff);
    fireEmitter2->setForce(0.5f,1.5f);

    SPK::StraightEmitter* fireEmitter3 = SPK::StraightEmitter::create(SPK::Vector3D(-0.6f,-0.8f,0.8f));
    fireEmitter3->setZone(SPK::Sphere::create(SPK::Vector3D(-0.375f,-0.375f,/*-1.15f*/base_z - .15f),0.3f * scale_coeff));
    fireEmitter3->setFlow(15*scale_coeff);
    fireEmitter3->setForce(0.5f,1.5f);

    SPK::StraightEmitter* fireEmitter4 = SPK::StraightEmitter::create(SPK::Vector3D(-0.8f,0.2f,0.5f));
    fireEmitter4->setZone(SPK::Sphere::create(SPK::Vector3D(-0.255f,0.225f,/*-1.2f*/base_z - .2f),0.2f* scale_coeff));
    fireEmitter4->setFlow(10*scale_coeff);
    fireEmitter4->setForce(0.5f,1.5f);

    SPK::StraightEmitter* fireEmitter5 = SPK::StraightEmitter::create(SPK::Vector3D(0.1f,-1.0f,0.8f));
    fireEmitter5->setZone(SPK::Sphere::create(SPK::Vector3D(-0.075f,-0.3f,/*-1.2f*/base_z - .2f),0.2f * scale_coeff ));
    fireEmitter5->setFlow(10*scale_coeff);
    fireEmitter5->setForce(0.5f,1.5f);

    SPK::SphericEmitter* smokeEmitter = SPK::SphericEmitter::create(SPK::Vector3D(0.0f,0.0f,1.0f),0.0f,0.5f * osg::PI*scale_coeff);
    smokeEmitter->setZone(SPK::Sphere::create(SPK::Vector3D(),1.2f * scale_coeff));
    FIXME(Tests)
    smokeEmitter->setFlow(/*100*/25*scale_coeff);
    smokeEmitter->setForce(0.5f,1.0f);

    // Groups
    SPK::Group* fireGroup = SPK::Group::create(fireModel,135);
    fireGroup->addEmitter(fireEmitter1);
    fireGroup->addEmitter(fireEmitter2);
    fireGroup->addEmitter(fireEmitter3);
    fireGroup->addEmitter(fireEmitter4);
    fireGroup->addEmitter(fireEmitter5);
    fireGroup->setRenderer(fireRenderer);
    fireGroup->setGravity(SPK::Vector3D(0.0f,0.0f,3.0f));
    FIXME(Tests)
    SPK::Group* smokeGroup = SPK::Group::create(smokeModel,135/*500000*/);
    smokeGroup->addEmitter(smokeEmitter);
    smokeGroup->setRenderer(smokeRenderer);
    smokeGroup->setGravity(SPK::Vector3D(0.0f,0.0f,0.4f));
    smokeGroup->enableSorting(true);

    // System
    SPK::System* particleSystem = SPK::System::create();
    particleSystem->addGroup(smokeGroup);
    particleSystem->addGroup(fireGroup);
    return particleSystem->getSPKID();
}


SPK::SPK_ID createFireSmoke( const SparkDrawable::TextureIDMap& textureIDMap, int screenWidth, int screenHeight, float scale_coeff )
{
    SparkDrawable::TextureIDMap::const_iterator itr;
    GET_TEXTURE_ID( "fire", textureFire );
    GET_TEXTURE_ID( "smoke", textureSmoke );

    // Renderers
    SPK::GL::GLQuadRenderer* fireRenderer = SPK::GL::GLQuadRenderer::create();
    fireRenderer->setScale(0.3f*scale_coeff,0.3f*scale_coeff);
    fireRenderer->setTexturingMode(SPK::TEXTURE_2D);
    fireRenderer->setTexture(textureFire);
    fireRenderer->setTextureBlending(GL_MODULATE);
    fireRenderer->setBlending(SPK::BLENDING_ADD);
    fireRenderer->enableRenderingHint(SPK::DEPTH_WRITE,false);
    fireRenderer->setAtlasDimensions(2,2);

    SPK::GL::GLQuadRenderer* smokeRenderer = SPK::GL::GLQuadRenderer::create();
    smokeRenderer->setTexturingMode( SPK::TEXTURE_2D );
    smokeRenderer->setAtlasDimensions( 2, 2 );
    smokeRenderer->setTexture( textureSmoke );
    smokeRenderer->setTextureBlending( GL_MODULATE );
    smokeRenderer->setScale( 0.05f*20, 0.05f*20 );
    smokeRenderer->setBlending( /*SPK::BLENDING_ADD*/SPK::BLENDING_ALPHA );
    smokeRenderer->enableRenderingHint( SPK::DEPTH_WRITE, false );

    // Models
    SPK::Model* fireModel = SPK::Model::create(
        SPK::FLAG_RED | SPK::FLAG_GREEN | SPK::FLAG_BLUE | SPK::FLAG_ALPHA |
        SPK::FLAG_SIZE | SPK::FLAG_ANGLE | SPK::FLAG_TEXTURE_INDEX,
        SPK::FLAG_RED | SPK::FLAG_GREEN | SPK::FLAG_ALPHA | SPK::FLAG_ANGLE,
        SPK::FLAG_RED | SPK::FLAG_GREEN | SPK::FLAG_TEXTURE_INDEX | SPK::FLAG_ANGLE,
        SPK::FLAG_SIZE);
    fireModel->setParam(SPK::PARAM_RED,0.8f,0.9f,0.8f,0.9f);
    fireModel->setParam(SPK::PARAM_GREEN,0.5f,0.6f,0.5f,0.6f);
    fireModel->setParam(SPK::PARAM_BLUE,0.3f);
    fireModel->setParam(SPK::PARAM_ALPHA,0.4f,0.0f);
    fireModel->setParam(SPK::PARAM_ANGLE,0.0f,2.0f * osg::PI,0.0f,2.0f * osg::PI);
    fireModel->setParam(SPK::PARAM_TEXTURE_INDEX,0.0f,4.0f);
    fireModel->setLifeTime(1.0f,1.5f);

    SPK::Interpolator* interpolator = fireModel->getInterpolator(SPK::PARAM_SIZE);
    interpolator->addEntry(0.5f,2.0f,5.0f);
    interpolator->addEntry(1.0f,0.0f);

    // Creates the model
    SPK::Model* smokeModel = SPK::Model::create(
        SPK::FLAG_RED | SPK::FLAG_GREEN | SPK::FLAG_BLUE | SPK::FLAG_SIZE | SPK::FLAG_ALPHA | SPK::FLAG_TEXTURE_INDEX | SPK::FLAG_ANGLE,
        SPK::FLAG_RED | SPK::FLAG_GREEN | SPK::FLAG_SIZE | SPK::FLAG_ALPHA,
        SPK::FLAG_SIZE | SPK::FLAG_TEXTURE_INDEX | SPK::FLAG_ANGLE /*,
        SPK::FLAG_ALPHA*/);
    
    smokeModel->setParam(SPK::PARAM_RED,0.3f,0.2f);
    smokeModel->setParam(SPK::PARAM_GREEN,0.25f,0.2f);
    smokeModel->setParam(SPK::PARAM_BLUE,0.2f);

    smokeModel->setParam( SPK::PARAM_SIZE, 0.5f, 1.0f, 10.0f, 20.0f );
    smokeModel->setParam( SPK::PARAM_ALPHA, 1.0f, 0.0f );
    smokeModel->setParam( SPK::PARAM_ANGLE, 0.0f, 2.0f * osg::PI );
    smokeModel->setParam( SPK::PARAM_TEXTURE_INDEX, 0.0f, 4.0f );
    smokeModel->setLifeTime( /*2.0*/5.f, 15.0f );

    FIXME(Tests)
        const float base_z = -1.f;
    // Emitters
    // The emitters are arranged so that the fire looks realistic
    SPK::StraightEmitter* fireEmitter1 = SPK::StraightEmitter::create(SPK::Vector3D(0.0f,0.0f,1.0f));
    fireEmitter1->setZone(SPK::Sphere::create(SPK::Vector3D(0.0f,0.0f,/*-1.0f*/base_z),0.5f * scale_coeff));
    fireEmitter1->setFlow(40 *scale_coeff);
    fireEmitter1->setForce(1.0f,2.5f);

    SPK::StraightEmitter* fireEmitter2 = SPK::StraightEmitter::create(SPK::Vector3D(1.0f,0.0f,0.6f));
    fireEmitter2->setZone(SPK::Sphere::create(SPK::Vector3D(0.15f,0.075f,/*-1.2f*/base_z - .2f),0.1f * scale_coeff));
    fireEmitter2->setFlow(15*scale_coeff);
    fireEmitter2->setForce(0.5f,1.5f);

    SPK::StraightEmitter* fireEmitter3 = SPK::StraightEmitter::create(SPK::Vector3D(-0.6f,-0.8f,0.8f));
    fireEmitter3->setZone(SPK::Sphere::create(SPK::Vector3D(-0.375f,-0.375f,/*-1.15f*/base_z - .15f),0.3f * scale_coeff));
    fireEmitter3->setFlow(15*scale_coeff);
    fireEmitter3->setForce(0.5f,1.5f);

    SPK::StraightEmitter* fireEmitter4 = SPK::StraightEmitter::create(SPK::Vector3D(-0.8f,0.2f,0.5f));
    fireEmitter4->setZone(SPK::Sphere::create(SPK::Vector3D(-0.255f,0.225f,/*-1.2f*/base_z - .2f),0.2f* scale_coeff));
    fireEmitter4->setFlow(10*scale_coeff);
    fireEmitter4->setForce(0.5f,1.5f);

    SPK::StraightEmitter* fireEmitter5 = SPK::StraightEmitter::create(SPK::Vector3D(0.1f,-1.0f,0.8f));
    fireEmitter5->setZone(SPK::Sphere::create(SPK::Vector3D(-0.075f,-0.3f,/*-1.2f*/base_z - .2f),0.2f * scale_coeff ));
    fireEmitter5->setFlow(10*scale_coeff);
    fireEmitter5->setForce(0.5f,1.5f);

    // Emitter
    SPK::SphericEmitter* smokeEmitter = SPK::SphericEmitter::create(
        SPK::Vector3D(/*-1.0f*/0.0f, 0.0f, 1.0f), 0.0f, 0.1f * osg::PI );
    //    particleEmitter->setZone( SPK::Point::create(SPK::Vector3D(0.0f, 0.015f, 0.0f)) );
    smokeEmitter->setFlow( 250.0 );
    smokeEmitter->setForce( 1.5f, 1.5f );


    // Groups
    SPK::Group* fireGroup = SPK::Group::create(fireModel,135);
    fireGroup->addEmitter(fireEmitter1);
    fireGroup->addEmitter(fireEmitter2);
    fireGroup->addEmitter(fireEmitter3);
    fireGroup->addEmitter(fireEmitter4);
    fireGroup->addEmitter(fireEmitter5);
    fireGroup->setRenderer(fireRenderer);
    fireGroup->setGravity(SPK::Vector3D(0.0f,0.0f,3.0f));

    // Group
    SPK::Group* smokeGroup = SPK::Group::create( smokeModel, 1500 );
    smokeGroup->addEmitter( smokeEmitter );
    smokeGroup->setRenderer( smokeRenderer );
    smokeGroup->setGravity( SPK::Vector3D(0.0f, 0.0f, 0.05f) );
    smokeGroup->enableAABBComputing( true );

    // System
    SPK::System* particleSystem = SPK::System::create();
    particleSystem->addGroup(smokeGroup);
    particleSystem->addGroup(fireGroup);
    return particleSystem->getSPKID();
}