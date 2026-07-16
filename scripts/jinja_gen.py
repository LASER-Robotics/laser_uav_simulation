#!/usr/bin/env python3

import subprocess
import argparse
import jinja2
import os

parser = argparse.ArgumentParser()
parser.add_argument('--namespace', type=str, required=True)
parser.add_argument('--uav_model', type=str, required=True)
parser.add_argument('--instance', type=int, required=True)
parser.add_argument('--enable_ground_truth', action='store_true')
parser.add_argument('--enable_load_pendulum', action='store_true')
parser.add_argument('--enable_d435_front', action='store_true')
parser.add_argument('--enable_d435i_front', action='store_true')
parser.add_argument('--enable_d435i_down', action='store_true')
parser.add_argument('--enable_d435_down', action='store_true')
parser.add_argument('--enable_vio', action='store_true')
parser.add_argument('--enable_garmin', action='store_true')
parser.add_argument('--enable_livox', action='store_true')
parser.add_argument('--enable_livox_45', action='store_true')
parser.add_argument('--enable_livox_down', action='store_true')
parser.add_argument('--enable_livox_45_down', action='store_true')
parser.add_argument('--x', type=float, default=0.0)
parser.add_argument('--y', type=float, default=0.0)
parser.add_argument('--z', type=float, default=0.0)
parser.add_argument('--roll', type=float, default=0.0)
parser.add_argument('--pitch', type=float, default=0.0)
parser.add_argument('--yaw', type=float, default=0.0)

args = parser.parse_args()

base_dir = os.path.dirname(os.path.abspath(__file__))

snippets_dir = os.path.realpath(os.path.join(base_dir, '../models/laser_uavs_description'))
drones_dir = os.path.realpath(os.path.join(snippets_dir, 'sdf'))

# Configurar o carregador de arquivos Jinja2
env = jinja2.Environment(loader=jinja2.FileSystemLoader([snippets_dir, drones_dir]))

# Carregar o template principal
template = env.get_template(args.uav_model + '.sdf.jinja')


# Dados para preencher o template
data = {
    'namespace': str(args.namespace),
    'model_name': str(args.uav_model),
    'instance': int(args.instance),
    'enable_load_pendulum': args.enable_load_pendulum, 
    'enable_ground_truth': args.enable_ground_truth, 
    'enable_d435_front': args.enable_d435_front, 
    'enable_d435i_front': args.enable_d435i_front, 
    'enable_d435i_down': args.enable_d435i_down, 
    'enable_d435_down': args.enable_d435_down, 
    'enable_vio': args.enable_vio, 
    'enable_livox': args.enable_livox,
    'enable_garmin': args.enable_garmin,
    'enable_livox_45': args.enable_livox_45,
    'enable_livox_down': args.enable_livox_down,
    'enable_livox_45_down': args.enable_livox_45_down,
    'x_offset': -float(args.x),
    'y_offset': -float(args.y),
    'z_offset': -float(args.z),
    'roll_offset': -float(args.roll),
    'pitch_offset': -float(args.pitch),
    'yaw_offset': -float(args.yaw)
}

# Renderizar o template
output = template.render(data)

dir_path = '/tmp/laser_uavs_description/sdf'
file_path = os.path.join(dir_path, args.uav_model + '.sdf')

os.makedirs(dir_path, exist_ok=True)

# Salvando o arquivo SDF
with open('/tmp/laser_uavs_description/sdf/' + args.uav_model + '_' + str(args.instance) + '.sdf', 'w') as file:
    file.write(output)

print("Generation jinja is successfuly")
