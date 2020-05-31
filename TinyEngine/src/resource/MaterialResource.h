#ifndef RESOURCE_MATERIALRESOURCE_H_
#define RESOURCE_MATERIALRESOURCE_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "../composite/Composite.h"
#include "../math/Math.h"

struct MaterialUniformBuffer {
	glm::vec4 baseColorFactor = glm::vec4(1.0f);

	float metallicFactor = 1.0f;
	float roughnessFactor = 1.0f;
	float normalScale = 1.0f;
	float occlusionStrength = 1.0f;

	glm::vec3 emissiveFactor = glm::vec3(0.0f);
	uint32_t alphaMode = 0;

	float alphaCutoff = 0.5f;
	bool doubleSided = false;
};

struct RaytraceMaterialUniformBuffer {
	MaterialUniformBuffer materialUniformBuffer;

	int32_t baseColorTexture = -1;
	int32_t metallicRoughnessTexture = -1;

	int32_t emissiveTexture = -1;

	int32_t occlusionTexture = -1;

	int32_t normalTexture = -1;

	//

	int32_t padding;
};

struct MaterialResource {
	// Mapper helper

	MaterialUniformBuffer materialUniformBuffer;

	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;

	uint32_t binding = 0;

	// Rasterize helper

	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

	std::vector<VkDescriptorImageInfo> descriptorImageInfos;
	std::vector<VkDescriptorBufferInfo> descriptorBufferInfos;

	UniformBufferResource uniformBufferResource = {};

	std::map<std::string, std::string> macros;

};

#endif /* RESOURCE_MATERIALRESOURCE_H_ */
