#ifndef ROS2_QUADRUPED__QUAD_LOCAL_PLANNER__LOCAL_FOOTSTEP_PLANNER_HPP_
#define ROS2_QUADRUPED__QUAD_LOCAL_PLANNER__LOCAL_FOOTSTEP_PLANNER_HPP_


#include "quad_local_planner/local_planner_modes.hpp"

#include "rclcpp/rclcpp.hpp"
#include "nav_msgs/msg/path.hpp"
#include "tf2_ros/transform_listener.h"
#include "tf2_ros/buffer.h"
#include "tf2_ros/create_timer_ros.h"
#include "tf2_ros/transform_listener.h"
#include "tf2_ros/transform_broadcaster.h"

#include "quad_msgs/msg/foot_plan_discrete.hpp"
#include "quad_msgs/msg/foot_state.hpp"
#include "quad_msgs/msg/multi_foot_plan_continuous.hpp"
#include "quad_msgs/msg/multi_foot_plan_discrete.hpp"
#include "quad_msgs/msg/multi_foot_state.hpp"
#include "quad_msgs/msg/robot_plan.hpp"
#include "quad_msgs/msg/robot_state.hpp"

#include "quad_utils/fast_terrain_map.hpp"
#include "quad_utils/quad_kd.hpp"
#include "quad_utils/ros_utils.hpp"
#include "quad_utils/function_timer.hpp"

#include "Eigen/Eigen"
#include "grid_map_core/grid_map_core.hpp"
#include "grid_map_ros/GridMapRosConverter.hpp"
#include "grid_map_ros/grid_map_ros.hpp"


namespace ros2_quadruped {
namespace quad_local_planner {

//! A local footstep planning class for quad
/*!
   FootstepPlanner is a container for all of the logic utilized in the local
   footstep planning node. The implementation must provide a clean and high
   level interface to the core algorithm
*/
class LocalFootstepPlanner 
{
public:
    /**
     * @brief Constructor for LocalFootstepPlanner Class
     * @return Constructed object of type LocalFootstepPlanner
     */
    LocalFootstepPlanner();

    /**
     * @brief Set the temporal parameters of this object
     * @param[in] dt The duration of one timestep in the plan
     * @param[in] period The period of a gait cycle in number of timesteps
     * @param[in] horizon_length The length of the planning horizon in number of
     * timesteps
     */
    void setTemporalParams(double dt, int period, int horizon_length,
                           const std::vector<double> &duty_cycles,
                           const std::vector<double> &phase_offsets);

    /**
     * @brief Set the spatial parameters of this object
     * @param[in] ground_clearance The foot clearance over adjacent footholds in m
     * @param[in] hip_clearance The foot clearance under hip in m
     * @param[in] standing_error_threshold Threshold of body error from desired
     * goal to start stepping
     * @param[in] grf_weight Weight on GRF projection (0 to 1)
     * @param[in] kinematics Kinematics class for computations
     * @param[in] foothold_search_radius Radius to locally search for valid
     * footholds (m)
     * @param[in] foothold_obj_threshold Minimum objective function value for
     * valid foothold
     * @param[in] obj_fun_layer Terrain layer for foothold search
     * @param[in] toe_radius Toe radius
     */
    void setSpatialParams(double ground_clearance, double hip_clearance,
                         double grf_weight, double standing_error_threshold,
                         std::shared_ptr<quad_utils::QuadKD> kinematics,
                         double foothold_search_radius,
                         double foothold_obj_threshold,
                         std::string obj_fun_layer, double toe_radius);

    /**
     * @brief Transform a vector of foot positions from the world to the body
     * frame
     * @param[in] body_plan Current body plan
     * @param[in] foot_positions_world Foot positions in the world frame
     * @param[out] foot_positions_body Foot positions in the body frame
     */
    void getFootPositionsBodyFrame(const Eigen::VectorXd &body_plan,
                                   const Eigen::VectorXd &foot_positions_world,
                                   Eigen::VectorXd &foot_positions_body);

    /**
     * @brief Transform the entire foot plan from the world to the body frame
     * @param[in] body_plan Current body plan
     * @param[in] foot_positions_world Foot positions in the world frame
     * @param[out] foot_positions_body Foot positions in the body frame
     */
    void getFootPositionsBodyFrame(const Eigen::MatrixXd &body_plan,
                                   const Eigen::MatrixXd &foot_positions_world,
                                   Eigen::MatrixXd &foot_positions_body);

    /**
     * @brief Update the map of this object
     * @param[in] terrain The map of the terrain
     */
    void updateMap(const quad_utils::FastTerrainMap &terrain);

    /**
     * @brief Update the map of this object
     * @param[in] terrain The map of the terrain
     */
    void updateMap(const grid_map::GridMap &terrain);

    /**
     * @brief Compute the contact schedule based on the current phase
     * @param[in] current_plan_index_ Current index in the plan
     * @param[in] body_plan Current body plan
     * @param[in] ref_primitive_plan_ Reference primitive plan
     * @param[in] control_mode Control mode
     * @param[out] contact_schedule 2D array of contact states
     */
    void computeContactSchedule(int current_plan_index,
                                const Eigen::MatrixXd &body_plan,
                                const Eigen::VectorXi &ref_primitive_plan_,
                                int control_mode,
                                std::vector<std::vector<bool>> &contact_schedule);

    /**
     * @brief Update the discrete footstep plan with the current plan
     * @param[in] current_plan_index Current plan index
     * @param[in] contact_schedule Current contact schedule
     * @param[in] body_plan Current body plan
     * @param[in] grf_plan Current grf plan
     * @param[in] ref_body_plan Reference body plan
     * @param[in] foot_positions_current Current foot position in the world frame
     * @param[in] foot_velocities_current Current foot position in the world frame
     * @param[in] first_element_duration Duration of first element of horizon (may
     * not be dt)
     * @param[in] past_footholds_msg Message of past footholds, used for
     * interpolation of swing state
     * @param[out] foot_positions Foot positions over the horizon
     * @param[out] foot_velocities Foot velocities over the horizon
     * @param[out] foot_accelerations Foot accelerations over the horizon
     */
    void computeFootPlan(int current_plan_index,
                         const std::vector<std::vector<bool>> &contact_schedule,
                         const Eigen::MatrixXd &body_plan,
                         const Eigen::MatrixXd &grf_plan,
                         const Eigen::MatrixXd &ref_body_plan,
                         const Eigen::VectorXd &foot_positions_current,
                         const Eigen::VectorXd &foot_velocities_current,
                         double first_element_duration,
                         quad_msgs::msg::MultiFootState &past_footholds_msg,
                         Eigen::MatrixXd &foot_positions,
                         Eigen::MatrixXd &foot_velocities,
                         Eigen::MatrixXd &foot_accelerations);

    /**
     * @brief Convert the foot positions and contact schedule into ros messages
     * for the foot plan
     * @param[in] contact_schedule Current contact schedule
     * @param[in] current_plan_index Current plan index
     * @param[in] first_element_duration Duration of first element of horizon (may
     * not be dt)
     * @param[in] foot_positions Foot positions over the horizon
     * @param[in] foot_velocities Foot velocities over the horizon
     * @param[in] foot_accelerations Foot accelerations over the horizon
     * @param[out] future_footholds_msg Message for future (planned) footholds
     * @param[out] foot_plan_continuous_msg Message for continuous foot
     * trajectories
     */
    void loadFootPlanMsgs(
        const std::vector<std::vector<bool>> &contact_schedule,
        int current_plan_index, double first_element_duration,
        const Eigen::MatrixXd &foot_positions,
        const Eigen::MatrixXd &foot_velocities,
        const Eigen::MatrixXd &foot_accelerations,
        quad_msgs::msg::MultiFootPlanDiscrete &future_footholds_msg,
        quad_msgs::msg::MultiFootPlanContinuous &foot_plan_continuous_msg);

    inline void printContactSchedule(const std::vector<std::vector<bool>> &contact_schedule) 
    {
        for (int i = 0; i < contact_schedule.size(); i++) 
        {
            for (int j = 0; j < contact_schedule.at(i).size(); j++)
            {
                if (contact_schedule[i][j]) 
                {
                    printf("1 ");
                } else {
                    printf("0 ");
                }
            }
            printf("\n");
        }
    }

    inline double getTerrainHeight(double x, double y) 
    {
        grid_map::Position pos = {x, y};
        double height = this->terrain_grid_.atPosition(
            "z_smooth", terrain_grid_.getClosestPositionInMap(pos),
            grid_map::InterpolationMethods::INTER_LINEAR);
        return (height);
    }

    inline double getTerrainSlope(double x, double y, double dx, double dy) 
    {
        std::array<double, 3> surf_norm = this->terrain_.getSurfaceNormalFiltered(x, y);

        double denom = dx * dx + dy * dy;
        if (denom <= 0 || surf_norm[2] <= 0) {
            double default_pitch = 0;
            return default_pitch;
        } else {
            double v_proj = (dx * surf_norm[0] + dy * surf_norm[1]) / sqrt(denom);
            double pitch = atan2(v_proj, surf_norm[2]);
            return pitch;
        }
    }

    inline void getTerrainSlope(double x, double y, double yaw, double &roll, double &pitch) 
    {
        std::array<double, 3> surf_norm = this->terrain_.getSurfaceNormalFiltered(x, y);

        Eigen::Vector3d norm_vec(surf_norm.data());
        Eigen::Vector3d axis = Eigen::Vector3d::UnitZ().cross(norm_vec);
        double ang = acos(std::max(std::min(Eigen::Vector3d::UnitZ().dot(norm_vec), 1.), -1.));

        if (ang < 1e-3) {
            roll = 0;
            pitch = 0;
            return;
        } else {
            Eigen::Matrix3d rot(Eigen::AngleAxisd(yaw, Eigen::Vector3d::UnitZ()));
            axis = rot.transpose() * (axis / axis.norm());
            tf2::Quaternion quat(tf2::Vector3(axis(0), axis(1), axis(2)), ang);
            tf2::Matrix3x3 m(quat);
            double tmp;
            m.getRPY(roll, pitch, tmp);
        }
    }

    inline void setTerrainMap(const grid_map::GridMap &grid_map) 
    {
        terrain_grid_ = grid_map;
    }

    // Compute future states by integrating linear states (hold orientation
    // states)
    inline Eigen::VectorXd computeFutureBodyPlan(double step, const Eigen::VectorXd &body_plan) 
    {
        // Initialize vector
        Eigen::VectorXd future_body_plan = body_plan;

        // Integrate the linear state
        future_body_plan.segment(0, 3) = future_body_plan.segment(0, 3) + body_plan.segment(6, 3) * step * dt_;
        return future_body_plan;
    }

private:
    /**
     * @brief Update the continuous foot plan to match the discrete
     */
    void updateContinuousPlan();

    /**
     * @brief Compute the cubic hermite spline
     * @param[in] pos_prev Previous position
     * @param[in] vel_prev Previous velocity
     * @param[in] pos_next Next position
     * @param[in] vel_next Next velocity
     * @param[in] phase Interplation phase
     * @param[in] duration Interplation duration
     * @param[out] pos Interplated position
     * @param[out] vel Interplated velocity
     * @param[out] acc Interplated accleration
     */
    void cubicHermiteSpline(double pos_prev, double vel_prev, double pos_next,
                            double vel_next, double phase, double duration,
                            double &pos, double &vel, double &acc);

    /**
     * @brief Search locally around foothold for optimal location
     * @param[in] foot_position Foothold to optimize around
     * @param[in] foot_position_prev_solve Foothold in prior solve
     * @return Optimized foothold
     */
    Eigen::Vector3d getNearestValidFoothold(
        const Eigen::Vector3d &foot_position,
        const Eigen::Vector3d &foot_position_prev_solve) const;

    /**
     * @brief Compute minimum covering circle problem using Welzl's algorithm
     * @param[in] P Hip position in the plan
     * @param[in] R Vertex storeage for the circle
     * @return Center and radius of the circle
     */
    Eigen::Vector3d welzlMinimumCircle(std::vector<Eigen::Vector2d> P,
                                        std::vector<Eigen::Vector2d> R);

    /**
     * @brief Compute swing apex height
     * @param[in] leg_idx Leg index
     * @param[in] body_plan Body plan in the mid air index
     * @param[in] foot_position_prev Position of the previous foothold
     * @param[in] foot_position_next Position of the next foothold
     * @return Apex height
     */
    double computeSwingApex(int leg_idx, const Eigen::VectorXd &body_plan,
                            const Eigen::Vector3d &foot_position_prev,
                            const Eigen::Vector3d &foot_position_next);

    /**
     * @brief Extract foot data from the matrix
     */
    inline Eigen::Vector3d getFootData(const Eigen::MatrixXd &foot_state_vars, int horizon_index, int foot_index) 
    {
        return foot_state_vars.block<1, 3>(horizon_index, 3 * foot_index);
    }

    /**
     * @brief Check if a foot is in contact at a given index
     */
    inline bool isContact(const std::vector<std::vector<bool>> &contact_schedule,
        int horizon_index, int foot_index) 
    {
        return (contact_schedule.at(horizon_index).at(foot_index));
    }

    /**
     * @brief Check if a foot is newly in contact at a given index
     */
    inline bool isNewContact(const std::vector<std::vector<bool>> &contact_schedule, 
        int horizon_index, int foot_index) 
    {
        if (horizon_index == 0) return false;

        return (!isContact(contact_schedule, horizon_index - 1, foot_index) &&
                 isContact(contact_schedule, horizon_index, foot_index));
    }

    /**
     * @brief Check if a foot is newly in swing at a given index
     */
    inline bool isNewLiftoff(const std::vector<std::vector<bool>> &contact_schedule, 
        int horizon_index, int foot_index) 
    {
        if (horizon_index == 0) return false;

        return (isContact(contact_schedule, horizon_index - 1, foot_index) &&
                !isContact(contact_schedule, horizon_index, foot_index));
    }

    /**
     * @brief Compute the index of the next contact for a foot. If none exist
     * return the last.
     */
    inline int getNextContactIndex(const std::vector<std::vector<bool>> &contact_schedule, 
        int horizon_index, int foot_index) 
    {
        // Loop through the rest of this contact schedule, if a new contact is found
        // return its index
        for (int i_touchdown = horizon_index; i_touchdown < horizon_length_; i_touchdown++) 
        {
            if (isNewContact(contact_schedule, i_touchdown, foot_index)) 
            {
                return i_touchdown;
            }
        }

        // If no contact is found, return the last index in the horizon
        return (horizon_length_ - 1);
    }

    /**
     * @brief Compute the index of the next liftoff for a foot. If none exist
     * return the last.
     */
    inline int getNextLiftoffIndex(const std::vector<std::vector<bool>> &contact_schedule, 
        int horizon_index, int foot_index) 
    {
        // Loop through the rest of this contact schedule, if a new liftoff is found
        // return its index
        for (int i_liftoff = horizon_index; i_liftoff < horizon_length_; i_liftoff++) 
        {
            if (isNewLiftoff(contact_schedule, i_liftoff, foot_index)) 
            {
                return i_liftoff;
            }
        }

        // If no contact is found, return the last index in the horizon
        return (horizon_length_ - 1);
    }

    /// Struct for terrain map data
    quad_utils::FastTerrainMap terrain_;

    /// GridMap for terrain map data
    grid_map::GridMap terrain_grid_;

    /// Number of feet
    const int num_feet_ = 4;

    /// Timestep for one finite element
    double dt_;

    /// Gait period in timesteps
    int period_;

    /// Horizon length in timesteps
    int horizon_length_;

    /// Phase offsets for the touchdown of each foot
    std::vector<double> phase_offsets_ = {0, 0.5, 0.5, 0.0};

    /// Duty cycles for the stance duration of each foot
    std::vector<double> duty_cycles_ = {0.5, 0.5, 0.5, 0.5};

    /// Nominal contact schedule
    std::vector<std::vector<bool>> nominal_contact_schedule_;

    /// Ground clearance
    double ground_clearance_;

    /// Hip clearance
    double hip_clearance_;

    /// Weighting on the projection of the grf
    double grf_weight_;

    /// Primitive ids - CONNECT_STANCE TODO(yanhaoy, astutt) make these enums
    const int CONNECT_STANCE = 0;

    /// Primitive ids - LEAP_STANCE
    const int LEAP_STANCE = 1;

    /// Primitive ids - FLIGHT
    const int FLIGHT = 2;

    /// Primitive ids - LAND_STANCE
    const int LAND_STANCE = 3;

    /// QuadKD class
    std::shared_ptr<quad_utils::QuadKD> quadKD_;

    /// Threshold of body error from desired goal to start stepping
    double standing_error_threshold_ = 0;

    /// Radius to locally search for valid footholds (m)
    double foothold_search_radius_;

    /// Minimum objective function value for valid foothold
    double foothold_obj_threshold_;

    /// Terrain layer for foothold search
    std::string obj_fun_layer_;

    /// Toe radius
    double toe_radius_;
};

}  // namespace quad_local_planner
}  // namespace ros2_quadruped

#endif  // ROS2_QUADRUPED__QUAD_LOCAL_PLANNER__LOCAL_FOOTSTEP_PLANNER_HPP_