#ifndef ROS2_TUTORIALS__TUTORIALS_NAV2_UTILS_PLANNER_TEST_HPP_
#define ROS2_TUTORIALS__TUTORIALS_NAV2_UTILS_PLANNER_TEST_HPP_

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <algorithm>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "nav2_msgs/action/compute_path_to_pose.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_msgs/msg/string.hpp"
#include "nav_msgs/msg/path.hpp"
#include "nav2_msgs/msg/costmap.hpp"
#include "nav2_msgs/srv/get_costmap.hpp"
#include "nav2_util/costmap.hpp"
#include "nav2_util/node_thread.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "geometry_msgs/msg/point.hpp"
#include "geometry_msgs/msg/pose_array.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/transform_stamped.hpp"
#include "sensor_msgs/msg/point_cloud.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"
#include "tf2_msgs/msg/tf_message.hpp"
#include "nav2_planner/planner_server.hpp"
#include "tf2_ros/transform_broadcaster.h"
#include "visualization_msgs/msg/marker.hpp"

using namespace std::chrono_literals;

namespace ros2_tutorials
{
namespace nav2
{

class NavFnPlannerTester : public nav2_planner::PlannerServer
{
public:
  NavFnPlannerTester()
  : PlannerServer()
  {
  }

  void printCostmap()
  {
    // print costmap for debug
    for (size_t i = 0; i != costmap_->getSizeInCellsX() * costmap_->getSizeInCellsY(); i++) {
      if (i % costmap_->getSizeInCellsX() == 0) {
        std::cout << "" << std::endl;
      }
      std::cout << costmap_ros_->getCostmap()->getCharMap()[i] << " ";
    }
    std::cout << "" << std::endl;
  }

  void setCostmap(nav2_util::Costmap * costmap)
  {
    nav2_msgs::msg::CostmapMetaData prop;
    nav2_msgs::msg::Costmap cm = costmap->get_costmap(prop);
    prop = cm.metadata;
    costmap_ros_->getCostmap()->resizeMap(
      prop.size_x, prop.size_y,
      prop.resolution, prop.origin.position.x, prop.origin.position.x);
    // Volatile prevents compiler from treating costmap_ptr as unused or changing its address
    volatile unsigned char * costmap_ptr = costmap_ros_->getCostmap()->getCharMap();
    delete[] costmap_ptr;
    costmap_ptr = new unsigned char[prop.size_x * prop.size_y];
    std::copy(cm.data.begin(), cm.data.end(), costmap_ptr);
  }

  bool createPath(
    const geometry_msgs::msg::PoseStamped & goal,
    nav_msgs::msg::Path & path)
  {
    geometry_msgs::msg::PoseStamped start;
    if (!nav2_util::getCurrentPose(start, *tf_, "map", "base_link", 0.1)) {
      return false;
    }
    try {
      path = planners_["GridBased"]->createPlan(start, goal);
      // The situation when createPlan() did not throw any exception
      // does not guarantee that plan was created correctly.
      // So it should be checked additionally that path is correct.
      if (!path.poses.size()) {
        return false;
      }
    } catch (...) {
      return false;
    }
    return true;
  }

  bool createPlan(
    const geometry_msgs::msg::PoseStamped& start,
    const geometry_msgs::msg::PoseStamped& goal,
    nav_msgs::msg::Path & path)
  {
    try {
      path = planners_["GridBased"]->createPlan(start, goal);
      if (!path.poses.size()) {
        return false;
      }
    } catch (...) {
      return false;
    }
    return true;
  }

  void onCleanup(const rclcpp_lifecycle::State & state)
  {
    on_cleanup(state);
  }

  void onActivate(const rclcpp_lifecycle::State & state)
  {
    on_activate(state);
  }

  void onDeactivate(const rclcpp_lifecycle::State & state)
  {
    on_deactivate(state);
  }

  void onConfigure(const rclcpp_lifecycle::State & state)
  {
    on_configure(state);
  }
};

enum class TaskStatus : int8_t
{
  SUCCEEDED = 1,
  FAILED = 2,
  RUNNING = 3,
};

class PlannerTester : public rclcpp::Node
{
public:
  using ComputePathToPoseCommand = geometry_msgs::msg::PoseStamped;
  using ComputePathToPoseResult = nav_msgs::msg::Path;

  PlannerTester();
  ~PlannerTester();

  // Activate the tester before running tests
  void activate();
  void deactivate();

  // Loads the provided map and and generates a costmap from it.
  void loadDefaultMap();

  void loadMap(const std::string& pgm_filename);

  bool loadMapFromYaml(const std::string& yaml_file);

  // Alternatively, use a preloaded 10x10 costmap
  void loadSimpleCostmap(const nav2_util::TestCostmap & testCostmapType);

  // Runs a single test with default poses depending on the loaded map
  // Success criteria is a collision free path and a deviation to a
  // reference path smaller than a tolerance.
  bool defaultPlannerTest(
    ComputePathToPoseResult & path,
    const double deviation_tolerance = 1.0);

  // Runs multiple tests with random initial and goal poses
  bool defaultPlannerRandomTests(
    const unsigned int number_tests,
    const float acceptable_fail_ratio);

  bool createPlan(
    const geometry_msgs::msg::PoseStamped& start,
    const geometry_msgs::msg::PoseStamped& goal,
    ComputePathToPoseResult & path);

  void publishPath(const ComputePathToPoseResult & path);

private:
  void setCostmap();

  TaskStatus createPlan(
    const ComputePathToPoseCommand & goal,
    ComputePathToPoseResult & path
  );

  bool is_active_;
  bool map_set_;
  bool costmap_set_;
  bool using_fake_costmap_;

  // Parameters of the costmap
  bool trinary_costmap_;
  bool track_unknown_space_;
  int lethal_threshold_;
  int unknown_cost_value_;
  nav2_util::TestCostmap testCostmapType_;

  // The static map
  std::shared_ptr<nav_msgs::msg::OccupancyGrid> map_;

  // The costmap representation of the static map
  std::unique_ptr<nav2_util::Costmap> costmap_;

  // The global planner
  std::shared_ptr<NavFnPlannerTester> planner_tester_;

  // A thread for spinning the ROS node
  std::unique_ptr<nav2_util::NodeThread> spin_thread_;

  // Path publisher
  rclcpp::Publisher<ComputePathToPoseResult>::SharedPtr path_publisher_{nullptr};

  // The tester must provide the robot pose through a transform
  std::unique_ptr<geometry_msgs::msg::TransformStamped> base_transform_;
  std::shared_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;
  rclcpp::TimerBase::SharedPtr transform_timer_;
  void publishRobotTransform();
  void startRobotTransform();
  void updateRobotPosition(const geometry_msgs::msg::Point & position);

  // Occupancy grid publisher for visualization
  rclcpp::Publisher<nav_msgs::msg::OccupancyGrid>::SharedPtr map_pub_;
  rclcpp::TimerBase::SharedPtr map_timer_;
  rclcpp::WallRate map_publish_rate_;
  void mapCallback();

  // Executes a test run with the provided end points.
  // Success criteria is a collision free path.
  // TODO(orduno): #443 Assuming a robot the size of a costmap cell
  bool plannerTest(
    const geometry_msgs::msg::Point & robot_position,
    const ComputePathToPoseCommand & goal,
    ComputePathToPoseResult & path);

  bool isCollisionFree(const ComputePathToPoseResult & path);

  bool isWithinTolerance(
    const geometry_msgs::msg::Point & robot_position,
    const ComputePathToPoseCommand & goal,
    const ComputePathToPoseResult & path) const;

  bool isWithinTolerance(
    const geometry_msgs::msg::Point & robot_position,
    const ComputePathToPoseCommand & goal,
    const ComputePathToPoseResult & path,
    const double deviationTolerance,
    const ComputePathToPoseResult & reference_path) const;

  void printPath(const ComputePathToPoseResult & path) const;
};

}  // namespace nav2
}  // namespace ros2_tutorials

#endif  // ROS2_TUTORIALS__TUTORIALS_NAV2_UTILS_PLANNER_TEST_HPP_