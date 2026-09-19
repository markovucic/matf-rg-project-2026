//
// Created by W10 on 9/19/2026.
//

#ifndef RG_MODEL_H
#define RG_MODEL_H

#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#include "Mesh.h"

class Model
{
public:
    // Svi ucitani mesh-evi modela
    std::vector<Mesh> meshes;
    std::string directory;

    // Konstruktor: ocekuje putanju do 3D modela (.obj, .fbx, itd.)
    Model(const std::string &path)
    {
        loadModel(path);
    }

    // Iscrtava ceo model (sve njegove mesh-eve)
    void Draw(Shader &shader)
    {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].draw(shader);
    }

private:
    void loadModel(const std::string &path)
    {
        Assimp::Importer importer;
        // Ucitavamo model i automatski pretvaramo u poligone/triangulise i racunamo normale
        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }

        directory = path.substr(0, path.find_last_of('/'));
        processNode(scene->mRootNode, scene);
    }

    void processNode(aiNode *node, const aiScene *scene)
    {
        // Obrada svih mesh-eva na trenutnom cvoru
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }

        // Rekurzivna obrada dece cvorova
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;

        // 1. Prolazimo kroz sva temena mesh-a
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;

            // Pozicije
            vertex.Position.x = mesh->mVertices[i].x;
            vertex.Position.y = mesh->mVertices[i].y;
            vertex.Position.z = mesh->mVertices[i].z;

            // Normale
            if (mesh->HasNormals())
            {
                vertex.Normal.x = mesh->mNormals[i].x;
                vertex.Normal.y = mesh->mNormals[i].y;
                vertex.Normal.z = mesh->mNormals[i].z;
            }
            else
            {
                vertex.Normal = glm::vec3(0.0f);
            }

            // Teksturne koordinate
            if (mesh->mTextureCoords[0])
            {
                vertex.TexCoords.x = mesh->mTextureCoords[0][i].x;
                vertex.TexCoords.y = mesh->mTextureCoords[0][i].y;
            }
            else
            {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }

            vertices.push_back(vertex);
        }

        // 2. Prolazimo kroz sve stranice (poligone/trouglove) i uzimamo indekse
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // Vracamo kreirani Mesh (otvara se i popunjava VAO/VBO unutar Mesh konstruktora)
        return Mesh(vertices, indices, textures);
    }
};

#endif

#endif //RG_MODEL_H
