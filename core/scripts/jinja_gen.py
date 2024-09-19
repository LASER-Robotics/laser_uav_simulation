#!/usr/bin/env python3

import subprocess
import argparse
import jinja2
import os

parser = argparse.ArgumentParser()
parser.add_argument('namespace', type=str)
parser.add_argument('uav_model', type=str)
parser.add_argument('--enable_d435_front', action='store_true')
parser.add_argument('--enable_d455_down', action='store_true')
parser.add_argument('--enable_rplidar', action='store_true')

args = parser.parse_args()

base_dir = os.path.dirname(os.path.abspath(__file__))

snippets_dir = os.path.realpath(os.path.join(base_dir, '../models'))
drones_dir = os.path.realpath(os.path.join(snippets_dir, args.uav_model))

# Configurar o carregador de arquivos Jinja2
env = jinja2.Environment(loader=jinja2.FileSystemLoader([snippets_dir, drones_dir]))

# Carregar o template principal
template = env.get_template(args.uav_model + '.sdf.jinja')


# Dados para preencher o template
data = {
    'namespace': f'{args.namespace}',
    'model_name': f'{args.uav_model}',
    'enable_d435_front': args.enable_d435_front, 
    'enable_d455_down': args.enable_d455_down, 
    'enable_rplidar': args.enable_rplidar
}

# Renderizar o template
output = template.render(data)

# Salvando o arquivo SDF
with open((drones_dir + '/' + args.uav_model + '.sdf'), 'w') as file:
    file.write(output)
with open(('/tmp/' + args.uav_model + '.sdf'), 'w') as file:
    file.write(output)

# Path to the shell script
shell_script_path = './link_models.sh'
result = subprocess.run(['bash', shell_script_path], capture_output=True, text=True)
