#ifndef MAIN_H 1
#include <glm/vec3.hpp> 
#include <glm/vec4.hpp> 

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace std;


static void loadMesh(char* filename, const aiScene* scene, aiVector3D scene_min, aiVector3D scene_max, aiVector3D scene_center);

void iniciaLuzes();

#endif // !MAIN_H 1

