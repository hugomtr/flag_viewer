# Flag Viewer
![flag](screenshots/flag.png)

## Requirements

Pour exécuter le projet vous aurez besoin d'une distribution linux basé sur Debian ainsi qu'un compilateur gcc capable de gérer du code c++>=11.

## Installation

```
git clone --recurse-submodules https://github.com/hugomtr/flag_viewer.git
cd flag_viewer
mkdir build
cd build
cmake ..
make
```

et pour lancer la simulation OpenGL

```
./../bin/FlagSimulation
```

## Courte description 

La logique du code du drapeau se trouve dans le fichier Base/Flag.cpp. Il s'agit d'un maillage de particules avec des interactions entres particules simulés à l'aide d'un modèle de ressort très simplifié, voir
```
void Constraint::satisfyConstraint()
```
L'animation de vent fonctionne correctement mais il est souvent difficile de trouver les bonnes valeurs pour les paramètres de force et de masse qui ne font pas diverger le modèle. 
La force de vent est elle proportionnelle à la surface du triangle sur laquelle elle s'applique, voir
```
void Flag::addwindForce(const Vec3 direction)
```
Pour aller plus loin nous aurions pu ajouter une interface plus simple à prendre en main pour modifier les paramètres du modèle. Nous aurions pu aussi améliorer notre modèle de ressort  en prenant en compte une élasticité. 


Les inputs du modèle se changent directement au début du code du fichier src/main.cpp.
flag_width et flag_height correspondent respectivement à la largeur et hauteur du drapeau à l'écran. num_particles_width et num_particles_height correspondent respectivement au nombre de particules sur la largeur du drapeau et de particules sur la hauteur du drapeau: il y'a donc num_particles_height * num_particles_width particules au total qui composent le drapeau.

![command](screenshots/command.png)
## Commandes de deplacement

| Command | Description |
|:---------:|:-----------:|
| `Z` | Move camera Forward, direction = vec3(0,0,-1)|
| `D` | Move camera right,  direction = vec3(1,0,0)|
| `S` | Move camera left,  direction = vec3(-1,0,0) |
| `Q` | Move camera back, direction = vec3(0,0,1) |
| `ESC` | Exit |






