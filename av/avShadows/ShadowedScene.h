#pragma once

/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/


#include <osg/buffered_value>

#include "ShadowTechnique.h"
#include "ShadowSettings.h"

namespace avShadow {

/** ShadowedScene provides a mechansim for decorating a scene that the needs to have shadows cast upon it.*/
class /*OSGSHADOW_EXPORT*/ ShadowedScene : public osg::Group
{
    public:

        ShadowedScene(ShadowTechnique* st=0);

        ShadowedScene(const ShadowedScene& es, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);

        META_Node(avShadow, ShadowedScene);

        virtual void traverse(osg::NodeVisitor& nv);

        void setShadowSettings(ShadowSettings* ss);
        ShadowSettings* getShadowSettings() { return _shadowSettings.get(); }
        const ShadowSettings* getShadowSettings() const { return _shadowSettings.get(); }

        void setShadowTechnique(ShadowTechnique* technique);
        ShadowTechnique* getShadowTechnique() { return _shadowTechnique.get(); }
        const ShadowTechnique* getShadowTechnique() const { return _shadowTechnique.get(); }

        /** Clean scene graph from any shadow technique specific nodes, state and drawables.*/
        void cleanSceneGraph();

        /** Dirty any cache data structures held in the attached ShadowTechnqiue.*/
        void dirty();

        /** Resize any per context GLObject buffers to specified size. */
        virtual void resizeGLObjectBuffers(unsigned int maxSize);

        /** If State is non-zero, this function releases any associated OpenGL objects for
           * the specified graphics context. Otherwise, releases OpenGL objects
           * for all graphics contexts. */
        virtual void releaseGLObjects(osg::State* = 0) const;

    public:

        /** deprecated, moved to ShadowSettings.*/
        void setReceivesShadowTraversalMask(unsigned int mask) { if (_shadowSettings.valid()) _shadowSettings->setReceivesShadowTraversalMask(mask); }
        /** deprecated, moved to ShadowSettings.*/
        unsigned int getReceivesShadowTraversalMask() const { return _shadowSettings.valid() ? _shadowSettings->getReceivesShadowTraversalMask() : 0xffffffff; }

        /** deprecated, moved to ShadowSettings.*/
        void setCastsShadowTraversalMask(unsigned int mask) { if (_shadowSettings.valid()) _shadowSettings->setCastsShadowTraversalMask(mask);  }
        /** deprecated, moved to ShadowSettings.*/
        unsigned int getCastsShadowTraversalMask() const { return _shadowSettings.valid() ? _shadowSettings->getCastsShadowTraversalMask() : 0xffffffff; }

protected:

        virtual ~ShadowedScene();

        osg::ref_ptr<ShadowSettings>    _shadowSettings;
        osg::ref_ptr<ShadowTechnique>   _shadowTechnique;

};

}