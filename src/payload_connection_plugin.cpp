#include <algorithm>
#include <cmath>
#include <functional>
#include <string>

#include <gazebo/common/Events.hh>
#include <gazebo/common/Plugin.hh>
#include <gazebo/common/Time.hh>
#include <gazebo/gazebo.hh>
#include <gazebo/msgs/msgs.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo/transport/transport.hh>

namespace gazebo {

class PayloadConnectionPlugin : public ModelPlugin {
public:
  void Load(
      physics::ModelPtr model,
      sdf::ElementPtr sdf) override {
    model_ = model;
    world_ = model_->GetWorld();

    uav1_model_name_ =
        sdf->Get<std::string>("uav1_model");

    uav2_model_name_ =
        sdf->Get<std::string>("uav2_model");

    rope_length_ =
        sdf->Get<double>("rope_length");

    stiffness_ =
        sdf->Get<double>("stiffness");

    damping_ =
        sdf->Get<double>("damping");

    max_force_ =
        sdf->Get<double>("max_force");

    if (sdf->HasElement("visual_segments")) {
      rope_visual_segments_ =
          sdf->Get<unsigned int>("visual_segments");
    }

    if (sdf->HasElement("visual_radius")) {
      rope_visual_radius_ =
          sdf->Get<double>("visual_radius");
    }

    if (sdf->HasElement("visual_update_rate")) {
      const double update_rate =
          sdf->Get<double>("visual_update_rate");

      if (update_rate > 0.0) {
        visual_update_period_ =
            1.0 / update_rate;
      }
    }

    if (rope_visual_segments_ < 2) {
      rope_visual_segments_ = 2;
    }

    visual_node_.reset(
        new transport::Node());

    visual_node_->Init(
        world_->Name());

    visual_publisher_ =
        visual_node_->Advertise<msgs::Visual>(
            "~/visual");

    update_connection_ =
        event::Events::ConnectWorldUpdateBegin(
            std::bind(
                &PayloadConnectionPlugin::OnUpdate,
                this));

    gzmsg
        << "[PayloadConnectionPlugin] Plugin loaded.\n"
        << "  UAV 1: " << uav1_model_name_ << "\n"
        << "  UAV 2: " << uav2_model_name_ << "\n"
        << "  Rope length: " << rope_length_ << " m\n"
        << "  Visual segments: "
        << rope_visual_segments_ << "\n";
  }

private:
  bool FindLinks() {
    const auto uav1_model =
        world_->ModelByName(
            uav1_model_name_);

    const auto uav2_model =
        world_->ModelByName(
            uav2_model_name_);

    /*
     * O payload pode ser carregado antes dos drones.
     * Nesse caso, tentamos novamente no próximo passo.
     */
    if (!uav1_model || !uav2_model) {
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

    if (!uav1_body_ ||
        !uav2_body_ ||
        !uav1_attachment_ ||
        !uav2_attachment_ ||
        !payload_body_ ||
        !payload_attachment_1_ ||
        !payload_attachment_2_) {
      if (!link_error_reported_) {
        gzerr
            << "[PayloadConnectionPlugin] One or more required "
            << "links were not found.\n"
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

    const double distance =
        difference.Length();

    /*
     * A corda está frouxa e não exerce força.
     */
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

    /*
     * Modelo mola-amortecedor:
     *
     * F = k*x + c*dx/dt
     */
    double tension =
        stiffness_ * extension +
        damping_ * extension_velocity;

    /*
     * A corda pode puxar, mas não pode empurrar.
     */
    tension =
        std::max(0.0, tension);

    /*
     * Limita a força para melhorar a estabilidade numérica.
     */
    tension =
        std::min(tension, max_force_);

    const auto force =
        direction * tension;

    /*
     * Puxa a caixa em direção ao drone.
     */
    payload_body_->AddForceAtWorldPosition(
        force,
        payload_position);

    /*
     * Aplica força igual e oposta no drone.
     */
    uav_body->AddForceAtWorldPosition(
        force * -1.0,
        uav_position);
  }

  ignition::math::Vector3d CalculateRopePoint(
      const ignition::math::Vector3d &payload_point,
      const ignition::math::Vector3d &uav_point,
      const double parameter,
      const double sag) const {
    /*
     * Interpolação linear entre as extremidades.
     */
    const auto straight_point =
        payload_point +
        (uav_point - payload_point) *
            parameter;

    /*
     * Curva parabólica.
     *
     * parameter = 0.0 -> payload
     * parameter = 0.5 -> maior curvatura
     * parameter = 1.0 -> UAV
     */
    const double vertical_displacement =
        4.0 *
        sag *
        parameter *
        (1.0 - parameter);

    return straight_point -
           ignition::math::Vector3d(
               0.0,
               0.0,
               vertical_displacement);
  }

  double CalculateVisualRopeLength(
      const ignition::math::Vector3d &payload_point,
      const ignition::math::Vector3d &uav_point,
      const double sag) const {
    double total_length = 0.0;

    auto previous_point =
        payload_point;

    for (unsigned int index = 1;
         index <= rope_visual_segments_;
         ++index) {
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
          (current_point -
           previous_point)
              .Length();

      previous_point =
          current_point;
    }

    return total_length;
  }

  double CalculateRopeSag(
      const ignition::math::Vector3d &payload_point,
      const ignition::math::Vector3d &uav_point) const {
    const double direct_distance =
        (uav_point -
         payload_point)
            .Length();

    /*
     * Quando a distância é igual ou maior que o comprimento
     * nominal, a corda fica reta.
     */
    if (direct_distance >= rope_length_) {
      return 0.0;
    }

    /*
     * Encontra uma curvatura cuja soma dos segmentos seja
     * aproximadamente igual ao comprimento nominal da corda.
     */
    double minimum_sag = 0.0;
    double maximum_sag = rope_length_;

    for (unsigned int iteration = 0;
         iteration < 20;
         ++iteration) {
      const double candidate_sag =
          (minimum_sag +
           maximum_sag) /
          2.0;

      const double candidate_length =
          CalculateVisualRopeLength(
              payload_point,
              uav_point,
              candidate_sag);

      if (candidate_length <
          rope_length_) {
        minimum_sag =
            candidate_sag;
      } else {
        maximum_sag =
            candidate_sag;
      }
    }

    return
        (minimum_sag +
         maximum_sag) /
        2.0;
  }

  void UpdateCylinderVisual(
      const std::string &visual_name,
      const ignition::math::Vector3d &point_a,
      const ignition::math::Vector3d &point_b) {
    const auto difference =
        point_b - point_a;

    const double segment_length =
        difference.Length();

    if (segment_length < 1e-6) {
      return;
    }

    const auto center =
        (point_a + point_b) /
        2.0;

    const auto direction =
        difference /
        segment_length;

    /*
     * O cilindro é criado alinhado com o eixo Z.
     * Calculamos a orientação necessária para apontá-lo
     * na direção do próximo segmento.
     */
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
     * O visual será filho do box_link.
     * A pose deve ser convertida do frame world para o
     * frame local da caixa.
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

    visual_message.set_visible(true);
    visual_message.set_cast_shadows(false);

    msgs::Set(
        visual_message.mutable_pose(),
        ignition::math::Pose3d(
            relative_position,
            relative_rotation));

    auto geometry =
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
      const std::string &rope_name,
      const ignition::math::Vector3d &payload_point,
      const ignition::math::Vector3d &uav_point) {
    const double sag =
        CalculateRopeSag(
            payload_point,
            uav_point);

    auto previous_point =
        payload_point;

    for (unsigned int index = 1;
         index <= rope_visual_segments_;
         ++index) {
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
          std::to_string(index - 1);

      UpdateCylinderVisual(
          segment_name,
          previous_point,
          current_point);

      previous_point =
          current_point;
    }
  }

  void UpdateRopeVisuals() {
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

  void OnUpdate() {
    if (!initialized_) {
      if (!FindLinks()) {
        return;
      }
    }

    /*
     * Atualização física das cordas.
     */
    ApplyRopeForce(
        uav1_body_,
        uav1_attachment_,
        payload_attachment_1_);

    ApplyRopeForce(
        uav2_body_,
        uav2_attachment_,
        payload_attachment_2_);

    /*
     * Atualização visual limitada à frequência configurada.
     */
    const auto simulation_time =
        world_->SimTime();

    const bool simulation_was_reset =
        simulation_time <
        last_visual_update_time_;

    const bool update_period_elapsed =
        (simulation_time -
         last_visual_update_time_)
                .Double() >=
            visual_update_period_;

    if (simulation_was_reset ||
        update_period_elapsed) {
      UpdateRopeVisuals();

      last_visual_update_time_ =
          simulation_time;
    }
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

  transport::NodePtr visual_node_;
  transport::PublisherPtr visual_publisher_;

  std::string uav1_model_name_;
  std::string uav2_model_name_;

  double rope_length_{1.0};

  double stiffness_{200.0};
  double damping_{10.0};
  double max_force_{50.0};

  unsigned int rope_visual_segments_{20};

  double rope_visual_radius_{0.004};
  double visual_update_period_{1.0 / 20.0};

  common::Time last_visual_update_time_;

  bool initialized_{false};
  bool link_error_reported_{false};
};

GZ_REGISTER_MODEL_PLUGIN(
    PayloadConnectionPlugin)

}  // namespace gazebo
