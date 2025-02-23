#ifndef DECOMP_ROS__DECOMP_ROS_UTILS__MESH_VISUAL_HPP_
#define DECOMP_ROS__DECOMP_ROS_UTILS__MESH_VISUAL_HPP_

#include <decomp_geometry/polyhedron.h>
#include <Eigen/Eigenvalues>

#include <OgreManualObject.h>
#include <OgreMaterialManager.h>
#include <OgreSceneNode.h>
#include <Ogre.h>
#include <OgreQuaternion.h>


// #include <rviz/ogre_helpers/mesh_shape.h>

namespace decomp_rviz_plugins {
  class MeshVisual {
    public:
      MeshVisual(Ogre::SceneManager *scene_manager, Ogre::SceneNode *parent_node);

      virtual ~MeshVisual();

      void setMessage(const vec_E<vec_Vec3f> &bds);
      void setFramePosition(const Ogre::Vector3 &position);
      void setFrameOrientation(const Ogre::Quaternion &orientation);

      void setColor(float r, float g, float b, float a);

    private:
      // std::unique_ptr<rviz::MeshShape> obj_;

      Ogre::SceneNode *frame_node_;

      Ogre::SceneManager *scene_manager_;
  };
}

#endif  // DECOMP_ROS__DECOMP_ROS_UTILS__MESH_VISUAL_HPP_
