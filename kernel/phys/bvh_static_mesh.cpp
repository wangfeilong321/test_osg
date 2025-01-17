#include "stdafx.h"
#include <btBulletDynamicsCommon.h>
#include "bullet_helpers.h"

#include "bvh_static_mesh.h"
#include "rigid_body_info.h"

namespace phys
{
    bvh_static_mesh::bvh_static_mesh(system_impl_ptr sys, sensor_ptr s)
        : sys_(sys)
    {
        btAlignedObjectArray<btVector3> points;

        //for (size_t i = 0; i < s->chunks_count(); ++i)
        //{
        //    for (size_t j = 0; j < s->triangles_count(i); ++j)
        //    {
        //        cg::triangle_3f tr = s->triangle(i, j);

        //        if (s->material(i) == "ground" || s->material(i) == "building" || s->material(i) == "concrete")
        //            mesh_.addTriangle(to_bullet_vector3(tr[0]), to_bullet_vector3(tr[1]), to_bullet_vector3(tr[2]), false);
        //    }
        //}

        btMatrix3x3 m;
        m.setIdentity();

        FIXME(Need btBvhTriangleMeshShape shape)
        //shape_ = boost::shared_ptr<btCollisionShape>(new btBvhTriangleMeshShape(&mesh_, true));

        //shape_ = shared_ptr<btCollisionShape>(new btStaticPlaneShape(btVector3(0, 0, 1), 0));

        ////////////////////////////
        //      Stub for the ground
        // osg::Vec3 norm = plane.getNormal();
        btCollisionShape* groundShape = new btStaticPlaneShape( btVector3(0.0, 0.0, 1.0), 0 );
        btTransform groundTransform;
        groundTransform.setIdentity();

        btDefaultMotionState* motionState = new btDefaultMotionState(groundTransform);
        btRigidBody::btRigidBodyConstructionInfo rigidInfo( 0.0, motionState, groundShape, btVector3(0.0, 0.0, 0.0) );
        body_ = shared_ptr<btRigidBody>(new btRigidBody(rigidInfo)); 
        ////////////////////////////

        // body_  = shared_ptr<btRigidBody>(new btRigidBody(0, 0, &*shape_));
        body_->setRestitution(0.1f);
        body_->setCenterOfMassTransform(btTransform(m, to_bullet_vector3(cg::point_3())));
        body_->setFriction(0.99f);
        body_->setActivationState(DISABLE_SIMULATION);
        body_->setUserPointer(new rigid_body_user_info_t(rb_terrain));
        body_->setCollisionFlags(body_->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

        sys_->dynamics_world()->addRigidBody(&*body_);
    }

    bvh_static_mesh::~bvh_static_mesh()
    {
        if (body_)
            sys_->dynamics_world()->removeRigidBody(&*body_);
    }

}
