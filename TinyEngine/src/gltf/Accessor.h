#ifndef GLTF_ACCESSOR_H_
#define GLTF_ACCESSOR_H_

#include <cstdint>

#include "BufferView.h"

struct Accessor {
	int32_t bufferView = -1;
	size_t byteOffset = 0;
	size_t count = 1;
	int32_t componentType = -1;
	int32_t type = -1;

	// Generic helper

	size_t componentTypeSize = 0;
	bool componentTypeSigned = false;
	bool componentTypeInteger = false;

	size_t typeCount = 0;

	BufferView* pBufferView = nullptr;
};

#endif /* GLTF_ACCESSOR_H_ */
