#ifndef RENDER_ALLOCATIONMANAGER_H_
#define RENDER_ALLOCATIONMANAGER_H_

#include <cstdint>

#include "../composite/Composite.h"
#include "../gltf/GLTF.h"
#include "../resource/Resource.h"

class AllocationManager {

private:

	ResourceManager resourceManager;

public:

	AllocationManager();

	~AllocationManager();

	VkBuffer getBuffer(const Accessor& accessor);

	ResourceManager& getResourceManager();

	bool createSharedDataResource(const BufferView& bufferView, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool, bool useRaytrace);

	bool createTextureResource(uint64_t textureHandle, const TextureResourceCreateInfo& textureResourceCreateInfo, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool);

	bool addMaterialResource(uint64_t materialHandle, uint64_t textureHandle, uint32_t texCoord, uint32_t binding, const std::string& prefix);

	bool finalizeMaterialResource(uint64_t materialHandle, VkDevice device);

	bool addPrimitiveResource(uint64_t primitiveHandle, uint32_t attributeIndex, uint32_t typeCount, const std::string& prefix, std::map<std::string, std::string>& macros, VkFormat format, uint32_t stride, VkBuffer buffer, VkDeviceSize offset);

	bool finalizePrimitive(const Primitive& primitive, const GLTF& glTF, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool, uint32_t width, uint32_t height, VkRenderPass renderPass, VkSampleCountFlagBits samples, const VkDescriptorSetLayout* pSetLayouts, VkCullModeFlags cullMode, bool useRaytrace = false);

	bool finalizeWorld(const GLTF& glTF, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool, VkImageView imageView, bool useRaytrace = false);

	void terminate(VkDevice device);

};

#endif /* RENDER_ALLOCATIONMANAGER_H_ */
