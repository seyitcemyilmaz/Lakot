#ifndef LAKOT_MESH_H
#define LAKOT_MESH_H

#include <vector>

#include "IDrawable.h"

class Mesh : public IDrawable {
private:
	unsigned int mVAO;
	unsigned int mVBO;
	unsigned int mIBO;

	std::vector<float> mVertices;
	std::vector<unsigned int> mIndices;
public:
	Mesh();

	void setVertices(std::vector<float> pVertices);
	void setIndices(std::vector<unsigned int> pIndices);

	void initializeBuffers();

	void bind();

	void draw();
};

#endif
