from jinja2 import Template

# Carregando o arquivo de template Jinja
with open('../models/x500/x500.sdf.jinja', 'r') as file:
    template_str = file.read()

# Compilando o template
template = Template(template_str)

# Dados para preencher o template
data = {
    # 'model_name': 'meu_modelo',
    # 'is_static': True,
    # 'x': 0.0,
    # 'y': 0.0,
    # 'z': 0.0,
    # 'size_x': 1.0,
    # 'size_y': 1.0,
    # 'size_z': 1.0
}

# Renderizando o template com os dados
output_sdf = template.render(data)

# Salvando o arquivo SDF
with open('../models/x500/x500.sdf', 'w') as file:
    file.write(output_sdf)
