#ifndef SIMPLEMODEL_H
#define SIMPLEMODEL_H

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <learnopengl/shader.h>

#include <string>
#include <vector>

struct CustomVertex {
    glm::vec3 Position;
    glm::vec3 NormalOrColor;
    glm::vec2 TexCoords;
//    glm::vec3 Attribute3;
//    glm::vec3 Attribute4;
};

class SimpleModel
{
private:
    std::vector<CustomVertex> mVertices;
    std::vector<unsigned int> texIDs;
    std::string mTexture;
    bool hasNormCol=false, hasTexture=false, hasCubeMaps=false;
    unsigned int VBO, VAO;
//    unsigned int EBO;

    unsigned int loadTexture(const char *path, int wrapParam)
    {
        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
        if (data)
        {
            GLenum format;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            if(wrapParam == GL_REPEAT || wrapParam ==  GL_CLAMP_TO_EDGE)
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapParam);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapParam);
            }
            else
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
            }
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        }
        else
        {
            std::cout << "Texture failed to load at path: " << path << std::endl;
            stbi_image_free(data);
        }

        return textureID;
    }
    unsigned int loadCubemap(const vector<std::string> &faces)
    {
        stbi_set_flip_vertically_on_load(false);
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

        int width, height, nrChannels;
        for (unsigned int i = 0; i < faces.size(); i++)
        {
            unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
            if (data)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                stbi_image_free(data);
            }
            else
            {
                std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
                stbi_image_free(data);
            }
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        stbi_set_flip_vertically_on_load(true);
        return textureID;
    }

public:
    SimpleModel(const std::vector<float> &vertices, bool normCol=false, bool texture=false /*,bool att3=false, bool att4= false*/)
    : hasNormCol(normCol), hasTexture(texture)
    {
        int take = 3;
        if(normCol) { take+= 3; }
        if(texture) { take+= 2; }
//        if(att3) { take+=3; }
//        if(att4) { take+=3; }

        CustomVertex temp;
        glm::vec3 vector3;
        glm::vec2 vector2;
        for(int i=0; i<vertices.size(); i+=take)
        {
            auto first = vertices.cbegin() + i;
            auto last = vertices.cbegin() + i + take + 1;
            int ord = 3;
            std::vector<float> sliced(first, last);
            vector3.x = sliced[0];
            vector3.y = sliced[1];
            vector3.z = sliced[2];
            temp.Position = vector3;

            if(normCol)
            {
                vector3.x = sliced[ord++];
                vector3.y = sliced[ord++];
                vector3.z = sliced[ord++];
                temp.NormalOrColor = vector3;
            }

            if(texture)
            {
                vector2.x = sliced[ord++];
                vector2.y = sliced[ord++];
                temp.TexCoords = vector2;
            }
            mVertices.push_back(temp);
            // TODO: att3 and att4
        }

        // create buffers/arrays
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
//        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(CustomVertex), &mVertices[0], GL_STATIC_DRAW);

//        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
//        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // set the vertex attribute pointers
        // vertex Positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CustomVertex), (void*)0);
        // vertex normals or colors
        if(hasNormCol)
        {
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CustomVertex),
                                  (void *) offsetof(CustomVertex, NormalOrColor));
        }
        // vertex texture coords
        if(hasTexture)
        {
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(CustomVertex),
                                  (void *) offsetof(CustomVertex, TexCoords));
        }
//        if(att3)
//        {
//            glEnableVertexAttribArray(3);
//            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(CustomVertex),
//                                  (void *) offsetof(CustomVertex, Attribute3));
//        }
//        if(att4)
//        {
//            glEnableVertexAttribArray(4);
//            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(CustomVertex),
//                                  (void *) offsetof(CustomVertex, Attribute4));
//        }

        glBindVertexArray(0);
    }

    void Destroy()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

    void AddTexture(const std::string &path, const std::string &name, int value, Shader &shader, int wrapParam=0)
    {
        unsigned int textureID = loadTexture(FileSystem::getPath(path).c_str(), wrapParam);
        texIDs.push_back(textureID);
        shader.use();
        shader.setInt(name, value);
    }

    void AddCubemaps(const vector<std::string> &faces, const std::string &name, int value, Shader &shader)
    {
        unsigned int skyboxID = loadCubemap(faces);
        texIDs.push_back(skyboxID);
        shader.use();
        shader.setInt(name, value);
        hasCubeMaps = true;
    }

    void Draw(int mode, bool test=false) const
    {
        glBindVertexArray(VAO);
        if(hasTexture or hasCubeMaps)
        {
            for(int i=0; i<texIDs.size(); i++)
            {
                glActiveTexture(GL_TEXTURE0+i);
                glBindTexture(hasCubeMaps ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, texIDs[i]);
            }
        }
        glDrawArrays(mode, 0, mVertices.size());

        // set back to default
        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
    }
};


#endif //SIMPLEMODEL_H
