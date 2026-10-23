#!/usr/bin/env python3

import argparse
import os
import sys

import jinja2


MODELS_WITHOUT_FLIGHT_CONTROLLER = {
    "payload",
    "sphere_model",
}


parser = argparse.ArgumentParser(
    description="Generate UAV SDF models using Jinja2 templates."
)

parser.add_argument(
    "--namespace",
    type=str,
    required=True,
    help="Robot namespace.",
)

parser.add_argument(
    "--uav_model",
    type=str,
    required=True,
    help="UAV or simulation model name.",
)

parser.add_argument(
    "--instance",
    type=int,
    required=True,
    help="UAV instance number.",
)

parser.add_argument(
    "--flight_controller",
    type=str,
    choices=["px4", "ap", "none"],
    default="none",
    help=(
        "Flight controller. Use 'none' for payloads "
        "and force-controlled spherical models."
    ),
)

parser.add_argument(
    "--enable_ground_truth",
    action="store_true",
)

parser.add_argument(
    "--enable_load_pendulum",
    action="store_true",
)

parser.add_argument(
    "--enable_d435_front",
    action="store_true",
)

parser.add_argument(
    "--enable_d435i_front",
    action="store_true",
)

parser.add_argument(
    "--enable_d435i_down",
    action="store_true",
)

parser.add_argument(
    "--enable_d435_down",
    action="store_true",
)

parser.add_argument(
    "--enable_vio",
    action="store_true",
)

parser.add_argument(
    "--enable_garmin",
    action="store_true",
)

parser.add_argument(
    "--enable_livox",
    action="store_true",
)

parser.add_argument(
    "--enable_livox_45",
    action="store_true",
)

parser.add_argument(
    "--enable_livox_down",
    action="store_true",
)

parser.add_argument(
    "--x",
    type=float,
    default=0.0,
)

parser.add_argument(
    "--y",
    type=float,
    default=0.0,
)

parser.add_argument(
    "--z",
    type=float,
    default=0.0,
)

parser.add_argument(
    "--roll",
    type=float,
    default=0.0,
)

parser.add_argument(
    "--pitch",
    type=float,
    default=0.0,
)

parser.add_argument(
    "--yaw",
    type=float,
    default=0.0,
)

args = parser.parse_args()


model_requires_flight_controller = (
    args.uav_model not in
    MODELS_WITHOUT_FLIGHT_CONTROLLER
)

if (
    model_requires_flight_controller and
    args.flight_controller == "none"
):
    parser.error(
        f"The model '{args.uav_model}' requires "
        "--flight_controller px4 or "
        "--flight_controller ap."
    )

if (
    not model_requires_flight_controller and
    args.flight_controller != "none"
):
    parser.error(
        f"The model '{args.uav_model}' does not use "
        "a flight controller. Use "
        "--flight_controller none or omit the argument."
    )


base_dir = os.path.dirname(
    os.path.abspath(__file__)
)

snippets_dir = os.path.realpath(
    os.path.join(
        base_dir,
        "../models/laser_uavs_description",
    )
)

drones_dir = os.path.realpath(
    os.path.join(
        snippets_dir,
        "sdf",
    )
)

env = jinja2.Environment(
    loader=jinja2.FileSystemLoader(
        [
            snippets_dir,
            drones_dir,
        ]
    )
)

template_name = (
    args.uav_model +
    ".sdf.jinja"
)

try:
    template = env.get_template(
        template_name
    )
except jinja2.exceptions.TemplateNotFound:
    print(
        f"[ERROR] Template '{template_name}' "
        f"not found in '{drones_dir}'.",
        file=sys.stderr,
    )

    sys.exit(1)


data = {
    "namespace": str(args.namespace),
    "model_name": str(args.uav_model),
    "instance": int(args.instance),
    "flight_controller": args.flight_controller,
    "enable_load_pendulum": args.enable_load_pendulum,
    "enable_ground_truth": args.enable_ground_truth,
    "enable_d435_front": args.enable_d435_front,
    "enable_d435i_front": args.enable_d435i_front,
    "enable_d435i_down": args.enable_d435i_down,
    "enable_d435_down": args.enable_d435_down,
    "enable_vio": args.enable_vio,
    "enable_livox": args.enable_livox,
    "enable_garmin": args.enable_garmin,
    "enable_livox_45": args.enable_livox_45,
    "enable_livox_down": args.enable_livox_down,
    "x_offset": -float(args.x),
    "y_offset": -float(args.y),
    "z_offset": -float(args.z),
    "roll_offset": -float(args.roll),
    "pitch_offset": -float(args.pitch),
    "yaw_offset": -float(args.yaw),
}

output = template.render(
    data
)

output_directory = (
    "/tmp/laser_uavs_description/sdf"
)

output_filename = (
    f"{args.uav_model}_"
    f"{args.instance}.sdf"
)

output_path = os.path.join(
    output_directory,
    output_filename,
)

os.makedirs(
    output_directory,
    exist_ok=True,
)

with open(
    output_path,
    "w",
    encoding="utf-8",
) as output_file:
    output_file.write(
        output
    )

print(
    "[SUCCESS] SDF file successfully generated at: "
    f"{output_path}"
)
