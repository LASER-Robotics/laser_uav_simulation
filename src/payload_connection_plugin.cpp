#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <string>

#include <gazebo/common/Events.hh>
#include <gazebo/common/Plugin.hh>
#include <gazebo/common/Time.hh>
#include <gazebo/gazebo.hh>
#include <gazebo/msgs/msgs.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo/transport/transport.hh>

#include <gazebo_ros/node.hpp>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/float64_multi_array.hpp>

namespace gazebo
{

class PayloadConnectionPlugin : public ModelPlugin
{
public:
  void Load(
    physics::ModelPtr model,
    sdf::ElementPtr sdf) override
  {
    model_ = model;
    world_ = model_->GetWorld();

    if (
      !sdf->HasElement("uav1_model") ||
      !sdf->HasElement("uav2_model"))
    {
      gzerr
        << "[PayloadConnectionPlugin] Missing required "
        << "uav1_model or uav2_model parameter.\n";

      return;
    }

    uav1_model_name_ =
      sdf->Get<std::string>(
      "uav1_model");

    uav2_model_name_ =
      sdf->Get<std::string>(
      "uav2_model");

    if (sdf->HasElement("training_center_model")) {
      training_center_model_ = sdf->Get<bool>("training_center_model");
    }

    if (sdf->HasElement("rope_length")) {
      rope_length_ =
        sdf->Get<double>(
        "rope_length");
    }

    if (sdf->HasElement("stiffness")) {
      stiffness_ =
        sdf->Get<double>(
        "stiffness");
    }

    if (sdf->HasElement("damping")) {
      damping_ =
        sdf->Get<double>(
        "damping");
    }

    if (sdf->HasElement("max_force")) {
      max_force_ =
        sdf->Get<double>(
        "max_force");
    }

    if (sdf->HasElement("visual_segments")) {
      rope_visual_segments_ =
        sdf->Get<unsigned int>(
        "visual_segments");
    }

    if (sdf->HasElement("visual_radius")) {
      rope_visual_radius_ =
        sdf->Get<double>(
        "visual_radius");
    }

    if (sdf->HasElement("visual_update_rate")) {
      const double update_rate =
        sdf->Get<double>(
        "visual_update_rate");

      if (update_rate > 0.0) {
        visual_update_period_ =
          1.0 / update_rate;
      }
    }

    if (sdf->HasElement("tension_topic")) {
      tension_topic_ =
        sdf->Get<std::string>(
        "tension_topic");
    }

    if (sdf->HasElement("tension_update_rate")) {
      const double update_rate =
        sdf->Get<double>(
        "tension_update_rate");

      if (update_rate > 0.0) {
        tension_update_period_ =
          1.0 / update_rate;
      }
    }

    if (rope_length_ <= 0.0) {
      gzerr
        << "[PayloadConnectionPlugin] rope_length must "
        << "be greater than zero.\n";

      return;
    }

    if (stiffness_ < 0.0 ||
      damping_ < 0.0)
    {
      gzerr
        << "[PayloadConnectionPlugin] stiffness and "
        << "damping cannot be negative.\n";

      return;
    }

    if (max_force_ <= 0.0) {
      gzerr
        << "[PayloadConnectionPlugin] max_force must "
        << "be greater than zero.\n";

      return;
    }

    if (rope_visual_segments_ < 2U) {
      rope_visual_segments_ = 2U;
    }

    /*
     * Gazebo transport node used for the rope visuals.
     */
    visual_node_.reset(
      new transport::Node());

    visual_node_->Init(
      world_->Name());

    visual_publisher_ =
      visual_node_->Advertise<
      msgs::Visual>(
      "~/visual");

    /*
     * ROS 2 node used for physical tension diagnostics.
     */
    ros_node_ =
      gazebo_ros::Node::Get(
      sdf);

    tension_publisher_ =
      ros_node_->create_publisher<
      std_msgs::msg::Float64MultiArray>(
      tension_topic_,
      rclcpp::QoS(10));

    /*
     * Connect only after all plugin resources have been
     * successfully initialized.
     */
    update_connection_ =
      event::Events::ConnectWorldUpdateBegin(
      std::bind(
        &PayloadConnectionPlugin::OnUpdate,
        this));

    gzmsg
      << "[PayloadConnectionPlugin] Plugin loaded.\n"
      << "  UAV 1: " << uav1_model_name_ << "\n"
      << "  UAV 2: " << uav2_model_name_ << "\n"
      << "  Training center model: " << training_center_model_ << "\n"
      << "  Rope length: " << rope_length_ << " m\n"
      << "  Stiffness: " << stiffness_ << " N/m\n"
      << "  Damping: " << damping_ << " Ns/m\n"
      << "  Maximum force: " << max_force_ << " N"
      << (training_center_model_ ? " (legacy mode only)\n" : "\n")
      << "  Tension topic: " << tension_topic_ << "\n"
      << "  Visual segments: "
      << rope_visual_segments_ << "\n";
  }

private:
  struct RopeForceResult
  {
    double distance{0.0};
    double extension{0.0};
    double axial_velocity{0.0};

    /*
     * Raw tension means after the unilateral-rope clamp
     * max(0, ...), but before max_force saturation.
     */
    double raw_tension{0.0};

    /*
     * Tension effectively applied to the Gazebo bodies.
     */
    double applied_tension{0.0};

    ignition::math::Vector3d
      force_on_payload{0.0, 0.0, 0.0};
  };

  bool FindLinks()
  {
    const auto uav1_model =
      world_->ModelByName(
      uav1_model_name_);

    const auto uav2_model =
      world_->ModelByName(
      uav2_model_name_);

    /*
     * The payload can be loaded before the UAVs. In that
     * case, search for the models again on the next step.
     */
    if (!uav1_model ||
      !uav2_model)
    {
      return false;
    }

    uav1_body_ =
      uav1_model->GetLink(
      "base_link");

    uav2_body_ =
      uav2_model->GetLink(
      "base_link");

    uav1_attachment_ =
      uav1_model->GetLink(
      "rope_attachment");

    uav2_attachment_ =
      uav2_model->GetLink(
      "rope_attachment");

    payload_body_ =
      model_->GetLink(
      "box_link");

    payload_attachment_1_ =
      model_->GetLink(
      "left_attachment");

    payload_attachment_2_ =
      model_->GetLink(
      "right_attachment");

    if (training_center_model_) {
      uav1_attachment_ = uav1_body_;
      uav2_attachment_ = uav2_body_;
      payload_attachment_1_ = payload_body_;
      payload_attachment_2_ = payload_body_;
    }

    if (
      !uav1_body_ ||
      !uav2_body_ ||
      !uav1_attachment_ ||
      !uav2_attachment_ ||
      !payload_body_ ||
      !payload_attachment_1_ ||
      !payload_attachment_2_)
    {
      if (!link_error_reported_) {
        gzerr
          << "[PayloadConnectionPlugin] One or more "
          << "required links were not found.\n"
          << "Required UAV links:\n"
          << "  base_link\n"
          << "  rope_attachment\n"
          << "Required payload links:\n"
          << "  box_link\n"
          << "  left_attachment\n"
          << "  right_attachment\n";

        link_error_reported_ = true;
      }

      return false;
    }

    initialized_ = true;

    gzmsg
      << "[PayloadConnectionPlugin] All links found.\n";

    return true;
  }

  RopeForceResult ApplyRopeForce(
    const physics::LinkPtr & uav_body,
    const physics::LinkPtr & uav_attachment,
    const physics::LinkPtr & payload_attachment)
  {
    RopeForceResult result;

    const auto uav_position =
      uav_attachment->WorldPose().Pos();

    const auto payload_position =
      payload_attachment->WorldPose().Pos();

    const auto difference =
      uav_position -
      payload_position;

    result.distance =
      difference.Length();

    if (!training_center_model_ && result.distance <= 1e-6) {
      return result;
    }

    /*
     * A slack rope does not generate spring or damping
     * forces.
     */
    if (!training_center_model_ && result.distance <= rope_length_) {
      return result;
    }

    const auto direction =
      difference /
      (result.distance + (training_center_model_ ? 1e-8 : 0.0));

    result.extension =
      std::max(0.0, result.distance - rope_length_);

    const auto relative_velocity =
      uav_attachment->WorldLinearVel() -
      payload_attachment->WorldLinearVel();

    result.axial_velocity =
      relative_velocity.Dot(
      direction);

    const double spring_force =
      stiffness_ *
      result.extension;

    const double damping_force =
      damping_ *
      result.axial_velocity;

    /*
     * The rope can pull, but cannot push.
     */
    result.raw_tension =
      std::max(
      0.0,
      spring_force +
      damping_force);

    /*
     * The training model does not saturate individual ropes. Preserve the
     * configured per-rope maximum only in legacy attachment mode.
     */
    result.applied_tension = training_center_model_ ? result.raw_tension :
      std::min(
      result.raw_tension,
      max_force_);

    result.force_on_payload =
      direction *
      result.applied_tension;

    /*
     * Pull the payload toward the UAV.
     */
    payload_body_->AddForceAtWorldPosition(
      result.force_on_payload,
      payload_position);

    /*
     * Apply the equal and opposite reaction on the UAV.
     */
    uav_body->AddForceAtWorldPosition(
      result.force_on_payload * -1.0,
      uav_position);

    return result;
  }

  void PublishTensionDiagnostics(
    const RopeForceResult & rope_1,
    const RopeForceResult & rope_2,
    const common::Time & simulation_time)
  {
    if (!tension_publisher_) {
      return;
    }

    const bool simulation_was_reset =
      simulation_time <
      last_tension_update_time_;

    const bool update_period_elapsed =
      (
      simulation_time -
      last_tension_update_time_
      ).Double() >=
      tension_update_period_;

    if (
      !simulation_was_reset &&
      !update_period_elapsed)
    {
      return;
    }

    const auto total_force =
      rope_1.force_on_payload +
      rope_2.force_on_payload;

    const double total_resultant_tension =
      total_force.Length();

    std_msgs::msg::Float64MultiArray message;

    /*
     * Message layout:
     *
     * [0]  rope 1 attachment distance [m]
     * [1]  rope 1 extension [m]
     * [2]  rope 1 axial velocity [m/s]
     * [3]  rope 1 unsaturated tension [N]
     * [4]  rope 1 applied tension [N]
     *
     * [5]  rope 2 attachment distance [m]
     * [6]  rope 2 extension [m]
     * [7]  rope 2 axial velocity [m/s]
     * [8]  rope 2 unsaturated tension [N]
     * [9]  rope 2 applied tension [N]
     *
     * [10] resultant force X [N]
     * [11] resultant force Y [N]
     * [12] resultant force Z [N]
     * [13] resultant tension norm [N]
     */
    message.data = {
      rope_1.distance,
      rope_1.extension,
      rope_1.axial_velocity,
      rope_1.raw_tension,
      rope_1.applied_tension,

      rope_2.distance,
      rope_2.extension,
      rope_2.axial_velocity,
      rope_2.raw_tension,
      rope_2.applied_tension,

      total_force.X(),
      total_force.Y(),
      total_force.Z(),
      total_resultant_tension
    };

    tension_publisher_->publish(
      message);

    last_tension_update_time_ =
      simulation_time;
  }

  ignition::math::Vector3d CalculateRopePoint(
    const ignition::math::Vector3d & payload_point,
    const ignition::math::Vector3d & uav_point,
    const double parameter,
    const double sag) const
  {
    const auto straight_point =
      payload_point +
      (
      uav_point -
      payload_point
      ) * parameter;

    const double vertical_displacement =
      4.0 *
      sag *
      parameter *
      (1.0 - parameter);

    return
      straight_point -
      ignition::math::Vector3d(
      0.0,
      0.0,
      vertical_displacement);
  }

  double CalculateVisualRopeLength(
    const ignition::math::Vector3d & payload_point,
    const ignition::math::Vector3d & uav_point,
    const double sag) const
  {
    double total_length = 0.0;

    auto previous_point =
      payload_point;

    for (
      unsigned int index = 1U;
      index <= rope_visual_segments_;
      ++index)
    {
      const double parameter =
        static_cast<double>(index) /
        static_cast<double>(
        rope_visual_segments_);

      const auto current_point =
        CalculateRopePoint(
        payload_point,
        uav_point,
        parameter,
        sag);

      total_length +=
        (
        current_point -
        previous_point
        ).Length();

      previous_point =
        current_point;
    }

    return total_length;
  }

  double CalculateRopeSag(
    const ignition::math::Vector3d & payload_point,
    const ignition::math::Vector3d & uav_point) const
  {
    const double direct_distance =
      (
      uav_point -
      payload_point
      ).Length();

    /*
     * When the direct distance is greater than or equal
     * to the nominal length, the rope is straight.
     */
    if (direct_distance >= rope_length_) {
      return 0.0;
    }

    double minimum_sag = 0.0;
    double maximum_sag = rope_length_;

    /*
     * Binary search for a parabolic sag that produces a
     * visual curve approximately equal to rope_length_.
     */
    for (
      unsigned int iteration = 0U;
      iteration < 20U;
      ++iteration)
    {
      const double candidate_sag =
        (
        minimum_sag +
        maximum_sag
        ) / 2.0;

      const double candidate_length =
        CalculateVisualRopeLength(
        payload_point,
        uav_point,
        candidate_sag);

      if (candidate_length <
        rope_length_)
      {
        minimum_sag =
          candidate_sag;
      } else {
        maximum_sag =
          candidate_sag;
      }
    }

    return
      (
      minimum_sag +
      maximum_sag
      ) / 2.0;
  }

  void UpdateCylinderVisual(
    const std::string & visual_name,
    const ignition::math::Vector3d & point_a,
    const ignition::math::Vector3d & point_b)
  {
    const auto difference =
      point_b -
      point_a;

    const double segment_length =
      difference.Length();

    if (segment_length < 1e-6) {
      return;
    }

    const auto center =
      (
      point_a +
      point_b
      ) / 2.0;

    const auto direction =
      difference /
      segment_length;

    const double horizontal_length =
      std::sqrt(
      direction.X() *
      direction.X() +
      direction.Y() *
      direction.Y());

    const double pitch =
      std::atan2(
      horizontal_length,
      direction.Z());

    const double yaw =
      std::atan2(
      direction.Y(),
      direction.X());

    const ignition::math::Quaterniond
      world_rotation(
      0.0,
      pitch,
      yaw);

    /*
     * The visual is a child of box_link, so the pose must
     * be converted from world coordinates to the payload
     * link frame.
     */
    const auto box_pose =
      payload_body_->WorldPose();

    const auto relative_position =
      box_pose
      .Rot()
      .RotateVectorReverse(
      center -
      box_pose.Pos());

    const auto relative_rotation =
      box_pose.Rot().Inverse() *
      world_rotation;

    msgs::Visual visual_message;

    visual_message.set_name(
      payload_body_->GetScopedName() +
      "::" +
      visual_name);

    visual_message.set_parent_name(
      payload_body_->GetScopedName());

    visual_message.set_parent_id(
      payload_body_->GetId());

    visual_message.set_visible(
      true);

    visual_message.set_cast_shadows(
      false);

    msgs::Set(
      visual_message.mutable_pose(),
      ignition::math::Pose3d(
        relative_position,
        relative_rotation));

    auto * geometry =
      visual_message.mutable_geometry();

    geometry->set_type(
      msgs::Geometry::CYLINDER);

    geometry
    ->mutable_cylinder()
    ->set_radius(
      rope_visual_radius_);

    geometry
    ->mutable_cylinder()
    ->set_length(
      segment_length);

    visual_publisher_->Publish(
      visual_message);
  }

  void UpdateRopeVisual(
    const std::string & rope_name,
    const ignition::math::Vector3d & payload_point,
    const ignition::math::Vector3d & uav_point)
  {
    const double sag =
      CalculateRopeSag(
      payload_point,
      uav_point);

    auto previous_point =
      payload_point;

    for (
      unsigned int index = 1U;
      index <= rope_visual_segments_;
      ++index)
    {
      const double parameter =
        static_cast<double>(index) /
        static_cast<double>(
        rope_visual_segments_);

      const auto current_point =
        CalculateRopePoint(
        payload_point,
        uav_point,
        parameter,
        sag);

      const std::string segment_name =
        rope_name +
        "_segment_" +
        std::to_string(
        index - 1U);

      UpdateCylinderVisual(
        segment_name,
        previous_point,
        current_point);

      previous_point =
        current_point;
    }
  }

  void UpdateRopeVisuals()
  {
    if (!visual_publisher_) {
      return;
    }

    UpdateRopeVisual(
      "rope_1_visual",
      payload_attachment_1_
      ->WorldPose()
      .Pos(),
      uav1_attachment_
      ->WorldPose()
      .Pos());

    UpdateRopeVisual(
      "rope_2_visual",
      payload_attachment_2_
      ->WorldPose()
      .Pos(),
      uav2_attachment_
      ->WorldPose()
      .Pos());
  }

  void OnUpdate()
  {
    if (!initialized_) {
      if (!FindLinks()) {
        return;
      }
    }

    const RopeForceResult rope_1 =
      ApplyRopeForce(
      uav1_body_,
      uav1_attachment_,
      payload_attachment_1_);

    const RopeForceResult rope_2 =
      ApplyRopeForce(
      uav2_body_,
      uav2_attachment_,
      payload_attachment_2_);

    const auto simulation_time =
      world_->SimTime();

    PublishTensionDiagnostics(
      rope_1,
      rope_2,
      simulation_time);

    const bool simulation_was_reset =
      simulation_time <
      last_visual_update_time_;

    const bool update_period_elapsed =
      (
      simulation_time -
      last_visual_update_time_
      ).Double() >=
      visual_update_period_;

    if (
      simulation_was_reset ||
      update_period_elapsed)
    {
      UpdateRopeVisuals();

      last_visual_update_time_ =
        simulation_time;
    }
  }

  physics::ModelPtr model_;
  physics::WorldPtr world_;

  physics::LinkPtr uav1_body_;
  physics::LinkPtr uav2_body_;

  physics::LinkPtr uav1_attachment_;
  physics::LinkPtr uav2_attachment_;

  physics::LinkPtr payload_body_;

  physics::LinkPtr payload_attachment_1_;
  physics::LinkPtr payload_attachment_2_;

  event::ConnectionPtr update_connection_;

  transport::NodePtr visual_node_;
  transport::PublisherPtr visual_publisher_;

  gazebo_ros::Node::SharedPtr ros_node_;

  rclcpp::Publisher<
    std_msgs::msg::Float64MultiArray
  >::SharedPtr tension_publisher_;

  std::string uav1_model_name_;
  std::string uav2_model_name_;

  std::string tension_topic_{
    "/payload/rope_tension"};

  // Matches unified_world cable law; physical attachment mode remains available.
  bool training_center_model_{false};
  double rope_length_{1.0};
  double stiffness_{200.0};
  double damping_{60.0};
  double max_force_{50.0};

  unsigned int rope_visual_segments_{20U};

  double rope_visual_radius_{0.004};
  double visual_update_period_{1.0 / 20.0};
  double tension_update_period_{1.0 / 100.0};

  common::Time last_visual_update_time_;
  common::Time last_tension_update_time_;

  bool initialized_{false};
  bool link_error_reported_{false};
};

GZ_REGISTER_MODEL_PLUGIN(
  PayloadConnectionPlugin)

}  // namespace gazebo
