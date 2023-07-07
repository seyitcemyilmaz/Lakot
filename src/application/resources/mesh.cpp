#include "Mesh.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

Mesh::Mesh() {
	mVAO = 0;
	mVBO = 0;
	mIBO = 0;
}

void Mesh::setVertices(std::vector<float> pVertices) {
	for (int i = 0; i < pVertices.size(); i++) {
		mVertices.push_back(pVertices[i]);
	}
}

void Mesh::setIndices(std::vector<unsigned int> pIndices) {
	for (int i = 0; i < pIndices.size(); i++) {
		mIndices.push_back(pIndices[i]);
	}
}

void Mesh::initializeBuffers() {
	glGenVertexArrays(1, &mVAO);
	glGenBuffers(1, &mVBO);
	glGenBuffers(1, &mIBO);

	glBindVertexArray(mVAO);

	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(float), mVertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * sizeof(unsigned int), mIndices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}

void Mesh::draw() {
	glBindVertexArray(mVAO);
	glDrawElements(GL_TRIANGLES, mIndices.size(), GL_UNSIGNED_INT, nullptr);
}
