#include <algorithm>
#include <functional>
#include <string>

#include <gazebo/common/Events.hh>
#include <gazebo/common/Plugin.hh>
#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>

namespace gazebo {

class PayloadConnectionPlugin : public ModelPlugin {
public:
  void Load(physics::ModelPtr model, sdf::ElementPtr sdf) override {
    model_ = model;
    world_ = model_->GetWorld();

    uav1_model_name_ = sdf->Get<std::string>("uav1_model");
    uav2_model_name_ = sdf->Get<std::string>("uav2_model");

    rope_length_ = sdf->Get<double>("rope_length");
    stiffness_ = sdf->Get<double>("stiffness");
    damping_ = sdf->Get<double>("damping");
    max_force_ = sdf->Get<double>("max_force");

    update_connection_ = event::Events::ConnectWorldUpdateBegin(
        std::bind(&PayloadConnectionPlugin::OnUpdate, this));

    gzmsg << "[PayloadConnectionPlugin] Plugin loaded.\n";
  }

private:
  bool FindLinks() {
    auto uav1_model = world_->ModelByName(uav1_model_name_);
    auto uav2_model = world_->ModelByName(uav2_model_name_);

    if (!uav1_model || !uav2_model) {
      return false;
    }

    uav1_body_ = uav1_model->GetLink("base_link");
    uav2_body_ = uav2_model->GetLink("base_link");

    uav1_attachment_ = uav1_model->GetLink("rope_attachment");
    uav2_attachment_ = uav2_model->GetLink("rope_attachment");

    payload_body_ = model_->GetLink("box_link");

    payload_attachment_1_ = model_->GetLink("left_attachment");
    payload_attachment_2_ = model_->GetLink("right_attachment");

    if (!uav1_body_ ||
        !uav2_body_ ||
        !uav1_attachment_ ||
        !uav2_attachment_ ||
        !payload_body_ ||
        !payload_attachment_1_ ||
        !payload_attachment_2_) {
      return false;
    }

    gzmsg << "[PayloadConnectionPlugin] All links found.\n";

    initialized_ = true;

    return true;
  }

  void ApplyRopeForce(
      const physics::LinkPtr &uav_body,
      const physics::LinkPtr &uav_attachment,
      const physics::LinkPtr &payload_attachment) {
    const auto uav_position =
        uav_attachment->WorldPose().Pos();

    const auto payload_position =
        payload_attachment->WorldPose().Pos();

    const auto difference =
        uav_position - payload_position;

    const double distance = difference.Length();

    // Rope is slack.
    if (distance <= rope_length_) {
      return;
    }

    const auto direction =
        difference / distance;

    const double extension =
        distance - rope_length_;

    const auto relative_velocity =
        uav_attachment->WorldLinearVel() -
        payload_attachment->WorldLinearVel();

    const double extension_velocity =
        relative_velocity.Dot(direction);

    double tension =
        stiffness_ * extension +
        damping_ * extension_velocity;

    // A rope can pull, but cannot push.
    tension = std::max(0.0, tension);

    // Safety limit against numerical instability.
    tension = std::min(tension, max_force_);

    const auto force =
        direction * tension;

    // Pull payload toward UAV.
    payload_body_->AddForceAtWorldPosition(
        force,
        payload_position);

    // Equal and opposite force on UAV.
    uav_body->AddForceAtWorldPosition(
        -force,
        uav_position);
  }

  void OnUpdate() {
    if (!initialized_) {
      if (!FindLinks()) {
        return;
      }
    }

    ApplyRopeForce(
        uav1_body_,
        uav1_attachment_,
        payload_attachment_1_);

    ApplyRopeForce(
        uav2_body_,
        uav2_attachment_,
        payload_attachment_2_);
  }

private:
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

  std::string uav1_model_name_;
  std::string uav2_model_name_;

  double rope_length_{1.0};
  double stiffness_{200.0};
  double damping_{10.0};
  double max_force_{50.0};

  bool initialized_{false};
};

GZ_REGISTER_MODEL_PLUGIN(PayloadConnectionPlugin)

}  // namespace gazebo
