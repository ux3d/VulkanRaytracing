#ifndef RENDER_RESOURCEMANAGER_H_
#define RENDER_RESOURCEMANAGER_H_

#include <map>

#include "../composite/Composite.h"

#include "../gltf/GLTF.h"

#include "BufferViewResource.h"
#include "MaterialResource.h"
#include "PrimitiveResource.h"
#include "SceneResource.h"
#include "GltfResource.h"

class ResourceManager {

private:

	std::map<const BufferView*, BufferViewResource> bufferViewResources;
	std::map<const Texture*, TextureResource> textureResources;
	std::map<const Material*, MaterialResource> materialResources;
	std::map<const Primitive*, PrimitiveResource> primitiveResources;
	std::map<const Scene*, SceneResource> sceneResources;
	GltfResource gltfResource;

	void terminate(BufferViewResource& bufferViewResource, VkDevice device);
	void terminate(TextureResource& textureResource, VkDevice device);
	void terminate(MaterialResource& materialResource, VkDevice device);
	void terminate(PrimitiveResource& primitiveResource, VkDevice device);
	void terminate(SceneResource& sceneResource, VkDevice device);

public:

	ResourceManager();

	~ResourceManager();

	//

	BufferViewResource* getBufferViewResource(const BufferView* bufferView);
	TextureResource* getTextureResource(const Texture* texture);
	MaterialResource* getMaterialResource(const Material* material);
	PrimitiveResource* getPrimitiveResource(const Primitive* primitive);
	SceneResource* getSceneResource(const Scene* scene);
	GltfResource* getGltfResource();

	//

	bool initBufferView(const BufferView& bufferView, const GLTF& glTF, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool, bool useRaytrace = false);
	bool resetBufferView(const BufferView& bufferView, VkDevice device);

	bool initMaterial(const Material& material, const GLTF& glTF, VkPhysicalDevice physicalDevice, VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>& descriptorSetLayoutBindings);
	bool resetMaterial(const Material& material, VkDevice device);

	bool initPrimitive(const Primitive& primitive, const GLTF& glTF, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool, uint32_t width, uint32_t height, VkRenderPass renderPass, VkSampleCountFlagBits samples, const VkDescriptorSetLayout* pSetLayouts, VkCullModeFlags cullMode, bool useRaytrace = false);
	bool resetPrimitive(const Primitive& Primitive, VkDevice device);

	bool initScene(const Scene& scene, const GLTF& glTF, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool, VkImageView imageView, bool useRaytrace = false);
	bool resetScene(const Scene& Primitive, VkDevice device);

	void terminate(VkDevice device);

};

#endif /* RENDER_RESOURCEMANAGER_H_ */
