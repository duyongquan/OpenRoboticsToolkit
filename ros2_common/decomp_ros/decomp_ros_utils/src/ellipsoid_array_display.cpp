#include "tf2_ros/transform_listener.h"
#include "tf2_ros/buffer.h"
#include "tf2_ros/create_timer_ros.h"
#include "tf2_ros/transform_listener.h"

#include "decomp_ros_utils/ellipsoid_array_display.hpp"

namespace decomp_rviz_plugins {

EllipsoidArrayDisplay::EllipsoidArrayDisplay() {
  color_property_ = new rviz_common::properties::ColorProperty("Color", QColor(204, 51, 204),
                                            "Color of ellipsoids.",
                                            this, SLOT(updateColorAndAlpha()));
  alpha_property_ = new rviz_common::properties::FloatProperty(
      "Alpha", 0.5, "0 is fully transparent, 1.0 is fully opaque.", this,
      SLOT(updateColorAndAlpha()));
}

void EllipsoidArrayDisplay::onInitialize() {
  MFDClass::onInitialize();
}

EllipsoidArrayDisplay::~EllipsoidArrayDisplay() {}

void EllipsoidArrayDisplay::reset() {
  MFDClass::reset();
  visual_ = nullptr;
}

void EllipsoidArrayDisplay::updateColorAndAlpha() {
  float alpha = alpha_property_->getFloat();
  Ogre::ColourValue color = color_property_->getOgreColor();

  if (visual_)
    visual_->setColor(color.r, color.g, color.b, alpha);
}

void EllipsoidArrayDisplay::processMessage(const decomp_ros_msgs::msg::EllipsoidArray::SharedPtr msg) {
  Ogre::Quaternion orientation;
  Ogre::Vector3 position;
  if (!context_->getFrameManager()->getTransform(
          msg->header.frame_id, msg->header.stamp, position, orientation)) {
    return;
  }

  std::shared_ptr<EllipsoidArrayVisual> visual;
  visual.reset(new EllipsoidArrayVisual(context_->getSceneManager(), scene_node_));

  visual->setMessage(msg);
  visual->setFramePosition(position);
  visual->setFrameOrientation(orientation);

  float alpha = alpha_property_->getFloat();
  Ogre::ColourValue color = color_property_->getOgreColor();
  visual->setColor(color.r, color.g, color.b, alpha);

  visual_ = visual;
}

}

// #include <pluginlib/class_list_macros.hpp>
// PLUGINLIB_EXPORT_CLASS(decomp_rviz_plugins::EllipsoidArrayDisplay, rviz_common::Display)
