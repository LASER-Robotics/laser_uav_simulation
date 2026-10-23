#!/bin/bash

# Atribuição dos argumentos
namespace=$1
uav_model=$2
x=$3
y=$4
z=$5
yaw=$6

# Passa o restante dos argumentos extras (se houver sensores, etc)
shift 6

# Extrai a instância baseada no namespace (ex: uav1 -> 0, uav2 -> 1)
instance=$(echo "$namespace" | sed -n 's/^uav\([0-9]\+\)$/\1/p')
instance=$((instance - 1))

JINJA_GEN="$(dirname "$(readlink -f "$0")")/jinja_gen.py"

echo "=========================================================="
echo " Gerando SDF do modelo esférico a partir do Jinja..."
echo "=========================================================="

"$JINJA_GEN" \
  --namespace "$namespace" \
  --uav_model "$uav_model" \
  --instance "$instance" \
  --x "$x" \
  --y "$y" \
  --z "$z" \
  --yaw "$yaw" \
  "$@"

echo "Aguardando o gzserver (Gazebo) iniciar..."
while ! pgrep -x "gzserver" > /dev/null; do
    sleep 1
done

# Um pequeno delay para garantir que a API do Gazebo está pronta
sleep 3 

# O nome exato que o seu PayloadConnectionPlugin vai procurar
model_full_name="${namespace}_${uav_model}_${instance}"
sdf_file="/tmp/laser_uavs_description/sdf/${uav_model}_${instance}.sdf"

echo "Fazendo o spawn do UAV Esférico: $model_full_name"
echo "Posição inicial -> x: $x, y: $y, z: $z, yaw: $yaw"

while gz model --verbose --spawn-file="$sdf_file" --model-name="$model_full_name" -x "$x" -y "$y" -z "$z" -Y "$yaw" | grep -q "An instance of Gazebo is not running."; do
	echo "O gzserver ainda não está aceitando conexões, tentando novamente..."
	sleep 1
done

echo "=========================================================="
echo " Spawn do $model_full_name concluído com sucesso!"
echo " Aguardando comandos Wrench do nó C++ (Padrão A)."
echo "=========================================================="
