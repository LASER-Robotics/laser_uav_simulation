#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include <gazebo/common/Events.hh>
#include <gazebo/common/Plugin.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo_ros/node.hpp>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/float64.hpp>
#include <std_msgs/msg/float64_multi_array.hpp>

namespace gazebo
{

class SphereForcePlugin : public ModelPlugin
{
public:
  SphereForcePlugin() = default;

  ~SphereForcePlugin() override
  {
    update_connection_.reset();
    action_subscription_.reset();
    gain_subscription_.reset();
    ros_node_.reset();
  }

  void Load(
    physics::ModelPtr model,
    sdf::ElementPtr sdf) override
  {
    model_ = model;
    world_ = model_->GetWorld();

    const std::string link_name =
      sdf->Get<std::string>(
      "link_name",
      "base_link").first;

    link_ = model_->GetLink(link_name);

    if (!link_) {
      gzerr
        << "[SphereForcePlugin] Link '"
        << link_name
        << "' was not found in model '"
        << model_->GetName()
        << "'.\n";

      return;
    }

    uav_index_ =
      sdf->Get<unsigned int>(
      "uav_index",
      0U).first;

    action_topic_ =
      sdf->Get<std::string>(
      "action_topic",
      "/uav1/action_out").first;

    gain_topic_ =
      sdf->Get<std::string>(
      "gain_topic",
      "/sphere_force/activation_gain").first;

    control_sensitivity_ =
      sdf->Get<double>(
      "control_sensitivity",
      15.0).first;

    const double initial_gain =
      sdf->Get<double>(
      "activation_gain",
      0.10).first;

    activation_gain_.store(
      std::clamp(
        initial_gain,
        0.0,
        1.0));

    command_timeout_seconds_ =
      sdf->Get<double>(
      "command_timeout",
      0.25).first;

    if (control_sensitivity_ <= 0.0) {
      gzerr
        << "[SphereForcePlugin] control_sensitivity must be positive.\n";

      return;
    }

    if (command_timeout_seconds_ <= 0.0) {
      gzerr
        << "[SphereForcePlugin] command_timeout must be positive.\n";

      return;
    }

    mass_ = link_->GetInertial()->Mass();

    if (mass_ <= 0.0) {
      gzerr
        << "[SphereForcePlugin] The controlled link mass must be positive.\n";

      return;
    }

    ros_node_ = gazebo_ros::Node::Get(sdf);

    action_subscription_ =
      ros_node_->create_subscription<
      std_msgs::msg::Float64MultiArray>(
      action_topic_,
      rclcpp::QoS(1).reliable(),
      std::bind(
        &SphereForcePlugin::ActionCallback,
        this,
        std::placeholders::_1));

    gain_subscription_ =
      ros_node_->create_subscription<
      std_msgs::msg::Float64>(
      gain_topic_,
      rclcpp::QoS(1).reliable(),
      std::bind(
        &SphereForcePlugin::GainCallback,
        this,
        std::placeholders::_1));

    update_connection_ =
      event::Events::ConnectWorldUpdateBegin(
      std::bind(
        &SphereForcePlugin::OnUpdate,
        this));

    RCLCPP_INFO(
      ros_node_->get_logger(),
      "Sphere force plugin ready: model=%s, index=%u, mass=%.3f kg, "
      "sensitivity=%.3f N, gain=%.3f, timeout=%.3f s, action_topic=%s",
      model_->GetName().c_str(),
      uav_index_,
      mass_,
      control_sensitivity_,
      activation_gain_.load(),
      command_timeout_seconds_,
      action_topic_.c_str());
  }

private:
  void ActionCallback(
    const std_msgs::msg::Float64MultiArray::SharedPtr message)
  {
    constexpr std::size_t kActionsPerUav = 3U;

    const std::size_t action_offset =
      kActionsPerUav *
      static_cast<std::size_t>(uav_index_);

    if (message->data.size() <
      action_offset + kActionsPerUav)
    {
      RCLCPP_ERROR(
        ros_node_->get_logger(),
        "Invalid joint action size: UAV index %u requires at least %zu "
        "values, but received %zu.",
        uav_index_,
        action_offset + kActionsPerUav,
        message->data.size());

      return;
    }

    std::array<double, 3> action{};

    for (std::size_t axis = 0;
      axis < action.size();
      ++axis)
    {
      action[axis] =
        std::clamp(
        message->data[action_offset + axis],
        -1.0,
        1.0);
    }

    {
      std::lock_guard<std::mutex> lock(
        command_mutex_);

      latest_action_ = action;
      last_command_time_ =
        std::chrono::steady_clock::now();
      command_received_ = true;
    }
  }

  void GainCallback(
    const std_msgs::msg::Float64::SharedPtr message)
  {
    const double requested_gain =
      message->data;

    if (!std::isfinite(requested_gain)) {
      RCLCPP_ERROR(
        ros_node_->get_logger(),
        "Activation gain must be finite.");

      return;
    }

    const double limited_gain =
      std::clamp(
      requested_gain,
      0.0,
      1.0);

    activation_gain_.store(
      limited_gain);

    RCLCPP_INFO(
      ros_node_->get_logger(),
      "Sphere PPO activation gain changed to %.3f.",
      limited_gain);
  }

  void OnUpdate()
  {
    std::array<double, 3> action{};
    bool command_is_fresh = false;

    {
      std::lock_guard<std::mutex> lock(
        command_mutex_);

      if (command_received_) {
        const double command_age =
          std::chrono::duration<double>(
          std::chrono::steady_clock::now() -
          last_command_time_).count();

        command_is_fresh =
          command_age <=
          command_timeout_seconds_;

        if (command_is_fresh) {
          action = latest_action_;
        }
      }
    }

    if (command_is_fresh != command_was_fresh_) {
      if (command_is_fresh) {
        RCLCPP_INFO(
          ros_node_->get_logger(),
          "Fresh PPO actions are being applied to %s.",
          model_->GetName().c_str());
      } else if (command_was_fresh_) {
        RCLCPP_WARN(
          ros_node_->get_logger(),
          "PPO action timeout for %s. Applying only the UAV hover force.",
          model_->GetName().c_str());
      }

      command_was_fresh_ =
        command_is_fresh;
    }

    const ignition::math::Vector3d gravity =
      world_->Gravity();

    // Gazebo applies mass * gravity automatically. This force
    // cancels only the controlled sphere's own weight, matching
    // UnifiedGenesisWorld's static UAV hover thrust.
    const ignition::math::Vector3d hover_force =
      gravity * -mass_;

    const double force_scale =
      activation_gain_.load() *
      control_sensitivity_;

    // PPO actions are Cartesian forces in the world frame.
    const ignition::math::Vector3d ppo_force(
      force_scale * action[0],
      force_scale * action[1],
      force_scale * action[2]);

    // AddForce() applies the force for the current physics step,
    // which is why this call belongs in the Gazebo update callback.
    link_->AddForce(
      hover_force + ppo_force);
  }

  physics::ModelPtr model_;
  physics::WorldPtr world_;
  physics::LinkPtr link_;

  gazebo_ros::Node::SharedPtr ros_node_;

  rclcpp::Subscription<
    std_msgs::msg::Float64MultiArray>::SharedPtr
  action_subscription_;

  rclcpp::Subscription<
    std_msgs::msg::Float64>::SharedPtr
  gain_subscription_;

  event::ConnectionPtr update_connection_;

  std::mutex command_mutex_;
  std::array<double, 3> latest_action_{};
  std::chrono::steady_clock::time_point
    last_command_time_{};

  std::atomic<double> activation_gain_{0.10};

  std::string action_topic_;
  std::string gain_topic_;

  unsigned int uav_index_{0U};

  double mass_{0.0};
  double control_sensitivity_{15.0};
  double command_timeout_seconds_{0.25};

  bool command_received_{false};
  bool command_was_fresh_{false};
};

GZ_REGISTER_MODEL_PLUGIN(SphereForcePlugin)

}  // namespace gazebo
