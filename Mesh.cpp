//
//  Mesh.cpp
//  Lab4
//
//  Created by CGIS on 27/10/2019.
//  Copyright © 2016 CGIS. All rights reserved.
//

#include "Mesh.hpp"
#include "glm/gtc/type_ptr.hpp"
namespace gps {

	/* Mesh Constructor */
	Mesh::Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices, std::vector<Texture> textures, std::vector<Material> colors)
	{
		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;
		this->colors = colors;
		this->setupMesh();
	}

	/* Mesh drawing function - also applies associated textures */
	void Mesh::Draw(gps::Shader shader)
	{
		shader.useShaderProgram();

		//set textures
		for (GLuint i = 0; i < textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i);
			glUniform1i(glGetUniformLocation(shader.shaderProgram, this->textures[i].type.c_str()), i);
			glBindTexture(GL_TEXTURE_2D, this->textures[i].id);
		}

		//set colors
		for (GLuint i = 0; i < colors.size(); i++)
		{
			glUniform3f(glGetUniformLocation(shader.shaderProgram, "ambient"), this->colors[i].ambient.x, this->colors[i].ambient.y, this->colors[i].ambient.z);
			glUniform3f(glGetUniformLocation(shader.shaderProgram, "diffuse"), this->colors[i].diffuse.x, this->colors[i].diffuse.y, this->colors[i].diffuse.z);
			glUniform3f(glGetUniformLocation(shader.shaderProgram, "specular"), this->colors[i].specular.x, this->colors[i].specular.y, this->colors[i].specular.z);
		}
		glBindVertexArray(this->VAO);
		glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		for (GLuint i = 0; i < this->textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

	}

	// Initializes all the buffer objects/arrays
	void Mesh::setupMesh(){
		// Create buffers/arrays
		glGenVertexArrays(1, &this->VAO);
		glGenBuffers(1, &this->VBO);
		glGenBuffers(1, &this->EBO);

		glBindVertexArray(this->VAO);
		// Load data into vertex buffers
		glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
		glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(Vertex), &this->vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(GLuint), &this->indices[0], GL_STATIC_DRAW);

		// Set the vertex attribute pointers
		// Vertex Positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
		// Vertex Normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Normal));
		// Vertex Texture Coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, TexCoords));

		glBindVertexArray(0);
	}

}
