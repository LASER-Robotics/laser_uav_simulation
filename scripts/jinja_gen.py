import jinja2
import os

base_dir = os.path.dirname(os.path.abspath(__file__))

snippets_dir = os.path.realpath(os.path.join(base_dir, '../models'))
drones_dir = os.path.realpath(os.path.join(snippets_dir, 'x500'))

# Configurar o carregador de arquivos Jinja2
env = jinja2.Environment(loader=jinja2.FileSystemLoader([snippets_dir, drones_dir]))

# Carregar o template principal
template = env.get_template('x500.sdf.jinja')

# Renderizar o template
output = template.render()

# Salvando o arquivo SDF
with open((drones_dir + '/x500.sdf'), 'w') as file:
    file.write(output)