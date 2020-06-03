#include "../shader/Shader.h"
#include "WorldBuilder.h"

WorldBuilder::WorldBuilder(ResourceManager& resourceManager, uint32_t width, uint32_t height, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool, VkRenderPass renderPass, VkSampleCountFlagBits samples, VkImageView imageView) :
	resourceManager(resourceManager), width(width), height(height), physicalDevice(physicalDevice), device(device), queue(queue), commandPool(commandPool), renderPass(renderPass), samples(samples), imageView(imageView)
{
}

bool WorldBuilder::buildBufferViews(const GLTF& glTF, bool useRaytrace)
{
	for (size_t i = 0; i < glTF.bufferViews.size(); i++)
	{
		const BufferView& bufferView = glTF.bufferViews[i];

		if (!createSharedDataResource(bufferView, physicalDevice, device, queue, commandPool, useRaytrace))
		{
			return false;
		}
	}

	return true;
}

bool WorldBuilder::buildAccessors(const GLTF& glTF, bool useRaytrace)
{
	for (size_t i = 0; i < glTF.accessors.size(); i++)
	{
		const Accessor& accessor = glTF.accessors[i];

		if (accessor.aliasedBuffer.byteLength > 0)
		{
			if (!createSharedDataResource(accessor.aliasedBufferView, physicalDevice, device, queue, commandPool, useRaytrace))
			{
				return false;
			}
		}
		else if (accessor.sparse.count >= 1)
		{
			if (!createSharedDataResource(accessor.sparse.bufferView, physicalDevice, device, queue, commandPool, useRaytrace))
			{
				return false;
			}
		}
	}

	return true;
}

bool WorldBuilder::buildTextures(const GLTF& glTF, bool useRaytrace)
{
	for (size_t i = 0; i < glTF.textures.size(); i++)
	{
		const Texture& texture = glTF.textures[i];

		uint64_t textureHandle = (uint64_t)&texture;

		textureHandles.push_back(textureHandle);

		TextureResourceCreateInfo textureResourceCreateInfo = {};
		textureResourceCreateInfo.imageDataResources = glTF.images[texture.source].imageDataResources;
		textureResourceCreateInfo.mipMap = true;

		if (texture.sampler >= 0)
		{
			textureResourceCreateInfo.samplerResourceCreateInfo.magFilter = glTF.samplers[texture.sampler].magFilter;
			textureResourceCreateInfo.samplerResourceCreateInfo.minFilter = glTF.samplers[texture.sampler].minFilter;
			textureResourceCreateInfo.samplerResourceCreateInfo.mipmapMode = glTF.samplers[texture.sampler].mipmapMode;
			textureResourceCreateInfo.samplerResourceCreateInfo.addressModeU = glTF.samplers[texture.sampler].addressModeU;
			textureResourceCreateInfo.samplerResourceCreateInfo.addressModeV = glTF.samplers[texture.sampler].addressModeV;
			textureResourceCreateInfo.samplerResourceCreateInfo.minLod = glTF.samplers[texture.sampler].minLod;
			textureResourceCreateInfo.samplerResourceCreateInfo.maxLod = glTF.samplers[texture.sampler].maxLod;
		}

		resourceManager.textureResourceSetCreateInformation(textureHandle, textureResourceCreateInfo);

		if (!resourceManager.textureResourceFinalize(textureHandle, physicalDevice, device, queue, commandPool))
		{
			return false;
		}
	}

	return true;
}

bool WorldBuilder::buildMaterials(const GLTF& glTF, bool useRaytrace)
{
	for (size_t i = 0; i < glTF.materials.size(); i++)
	{
		const Material& material = glTF.materials[i];

		materialHandles.push_back((uint64_t)&material);

		// Metallic Roughness
		if (material.pbrMetallicRoughness.baseColorTexture.index >= 0)
		{
			if (!resourceManager.materialResourceSetTextureResource(materialHandles[i], textureHandles[material.pbrMetallicRoughness.baseColorTexture.index], material.pbrMetallicRoughness.baseColorTexture.texCoord, "BASECOLOR"))
			{
				return false;
			}
		}

		if (material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
		{
			if (!resourceManager.materialResourceSetTextureResource(materialHandles[i], textureHandles[material.pbrMetallicRoughness.metallicRoughnessTexture.index], material.pbrMetallicRoughness.metallicRoughnessTexture.texCoord, "METALLICROUGHNESS"))
			{
				return false;
			}
		}

		// Base Material
		if (material.emissiveTexture.index >= 0)
		{
			if (!resourceManager.materialResourceSetTextureResource(materialHandles[i], textureHandles[material.emissiveTexture.index], material.emissiveTexture.texCoord, "EMISSIVE"))
			{
				return false;
			}
		}

		if (material.occlusionTexture.index >= 0)
		{
			if (!resourceManager.materialResourceSetTextureResource(materialHandles[i], textureHandles[material.occlusionTexture.index], material.occlusionTexture.texCoord, "OCCLUSION"))
			{
				return false;
			}
		}

		if (material.normalTexture.index >= 0)
		{
			if (!resourceManager.materialResourceSetTextureResource(materialHandles[i], textureHandles[material.normalTexture.index], material.normalTexture.texCoord, "NORMAL"))
			{
				return false;
			}
		}

		//

		MaterialUniformBuffer materialUniformBuffer = {};

		materialUniformBuffer.baseColorFactor = material.pbrMetallicRoughness.baseColorFactor;

		materialUniformBuffer.metallicFactor = material.pbrMetallicRoughness.metallicFactor;
		materialUniformBuffer.roughnessFactor = material.pbrMetallicRoughness.roughnessFactor;

		materialUniformBuffer.normalScale = material.normalTexture.scale;

		materialUniformBuffer.occlusionStrength = material.occlusionTexture.strength;

		materialUniformBuffer.emissiveFactor = material.emissiveFactor;

		materialUniformBuffer.alphaMode = material.alphaMode;
		materialUniformBuffer.alphaCutoff = material.alphaCutoff;

		materialUniformBuffer.doubleSided = material.doubleSided;

		resourceManager.materialResourceSetMaterialParameters(materialHandles[i], materialUniformBuffer, physicalDevice, device);

		if (!resourceManager.materialResourceFinalize(materialHandles[i], device))
		{
			return false;
		}
	}

	return true;
}

bool WorldBuilder::buildMeshes(const GLTF& glTF, bool useRaytrace)
{
	for (size_t i = 0; i < glTF.meshes.size(); i++)
	{
		const Mesh& mesh = glTF.meshes[i];

		uint64_t groupHandle = (uint64_t)&mesh;

		meshHandles.push_back(groupHandle);

		for (size_t k = 0; k < mesh.primitives.size(); k++)
		{
			const Primitive& primitive = mesh.primitives[k];

			uint64_t geometryHandle = (uint64_t)&primitive;
			uint64_t geometryModelHandle = (uint64_t)&primitive;

			VkCullModeFlags cullMode = VK_CULL_MODE_NONE;

			resourceManager.geometryResourceSetAttributesCount(geometryHandle, primitive.attributesCount);

			if (primitive.position >= 0)
			{
				const Accessor& accessor = glTF.accessors[primitive.position];

				uint32_t stride = HelperAccess::getStride(accessor);

				VkFormat format = VK_FORMAT_UNDEFINED;
				if (!HelperVulkan::getFormat(format, accessor.componentTypeSize, accessor.componentTypeSigned, accessor.componentTypeInteger, accessor.typeCount, accessor.normalized))
				{
					return false;
				}

				if (!resourceManager.geometryResourceSetAttribute(geometryHandle, glTF.accessors[primitive.position].count, glTF.accessors[primitive.position].typeCount, "POSITION", format, stride, resourceManager.getBuffer(getBufferHandle(accessor)), HelperAccess::getOffset(accessor), HelperAccess::getRange(accessor)))
				{
					return false;
				}
			}

			if (primitive.normal >= 0)
			{
				const Accessor& accessor = glTF.accessors[primitive.normal];

				uint32_t stride = HelperAccess::getStride(accessor);

				VkFormat format = VK_FORMAT_UNDEFINED;
				if (!HelperVulkan::getFormat(format, accessor.componentTypeSize, accessor.componentTypeSigned, accessor.componentTypeInteger, accessor.typeCount, accessor.normalized))
				{
					return false;
				}

				if (!resourceManager.geometryResourceSetAttribute(geometryHandle, glTF.accessors[primitive.normal].count, glTF.accessors[primitive.normal].typeCount, "NORMAL", format, stride, resourceManager.getBuffer(getBufferHandle(accessor)), HelperAccess::getOffset(accessor), HelperAccess::getRange(accessor)))
				{
					return false;
				}
			}

			if (primitive.tangent >= 0)
			{
				const Accessor& accessor = glTF.accessors[primitive.tangent];

				uint32_t stride = HelperAccess::getStride(accessor);

				VkFormat format = VK_FORMAT_UNDEFINED;
				if (!HelperVulkan::getFormat(format, accessor.componentTypeSize, accessor.componentTypeSigned, accessor.componentTypeInteger, accessor.typeCount, accessor.normalized))
				{
					return false;
				}

				if (!resourceManager.geometryResourceSetAttribute(geometryHandle, glTF.accessors[primitive.tangent].count, glTF.accessors[primitive.tangent].typeCount, "TANGENT", format, stride, resourceManager.getBuffer(getBufferHandle(accessor)), HelperAccess::getOffset(accessor), HelperAccess::getRange(accessor)))
				{
					return false;
				}
			}

			if (primitive.texCoord0 >= 0)
			{
				const Accessor& accessor = glTF.accessors[primitive.texCoord0];

				uint32_t stride = HelperAccess::getStride(accessor);

				VkFormat format = VK_FORMAT_UNDEFINED;
				if (!HelperVulkan::getFormat(format, accessor.componentTypeSize, accessor.componentTypeSigned, accessor.componentTypeInteger, accessor.typeCount, accessor.normalized))
				{
					return false;
				}

				if (!resourceManager.geometryResourceSetAttribute(geometryHandle, glTF.accessors[primitive.texCoord0].count, glTF.accessors[primitive.texCoord0].typeCount, "TEXCOORD_0", format, stride, resourceManager.getBuffer(getBufferHandle(accessor)), HelperAccess::getOffset(accessor), HelperAccess::getRange(accessor)))
				{
					return false;
				}
			}

			if (primitive.texCoord1 >= 0)
			{
				const Accessor& accessor = glTF.accessors[primitive.texCoord1];

				uint32_t stride = HelperAccess::getStride(accessor);

				VkFormat format = VK_FORMAT_UNDEFINED;
				if (!HelperVulkan::getFormat(format, accessor.componentTypeSize, accessor.componentTypeSigned, accessor.componentTypeInteger, accessor.typeCount, accessor.normalized))
				{
					return false;
				}

				if (!resourceManager.geometryResourceSetAttribute(geometryHandle, glTF.accessors[primitive.texCoord1].count, glTF.accessors[primitive.texCoord1].typeCount, "TEXCOORD_1", format, stride, resourceManager.getBuffer(getBufferHandle(accessor)), HelperAccess::getOffset(accessor), HelperAccess::getRange(accessor)))
				{
					return false;
				}
			}

			if (primitive.color0 >= 0)
			{
				const Accessor& accessor = glTF.accessors[primitive.color0];

				uint32_t stride = HelperAccess::getStride(accessor);

				VkFormat format = VK_FORMAT_UNDEFINED;
				if (!HelperVulkan::getFormat(format, accessor.componentTypeSize, accessor.componentTypeSigned, accessor.componentTypeInteger, accessor.typeCount, accessor.normalized))
				{
					return false;
				}

				if (!resourceManager.geometryResourceSetAttribute(geometryHandle, glTF.accessors[primitive.color0].count, glTF.accessors[primitive.color0].typeCount, "COLOR_0", format, stride, resourceManager.getBuffer(getBufferHandle(accessor)), HelperAccess::getOffset(accessor), HelperAccess::getRange(accessor)))
				{
					return false;
				}
			}

			if (primitive.joints0 >= 0)
			{
				const Accessor& accessor = glTF.accessors[primitive.joints0];

				uint32_t stride = HelperAccess::getStride(accessor);

				VkFormat format = VK_FORMAT_UNDEFINED;
				if (!HelperVulkan::getFormat(format, accessor.componentTypeSize, accessor.componentTypeSigned, accessor.componentTypeInteger, accessor.typeCount, accessor.normalized))
				{
					return false;
				}

				if (!resourceManager.geometryResourceSetAttribute(geometryHandle, glTF.accessors[primitive.joints0].count, glTF.accessors[primitive.joints0].typeCount, "JOINTS_0", format, stride, resourceManager.getBuffer(getBufferHandle(accessor)), HelperAccess::getOffset(accessor), HelperAccess::getRange(accessor)))
				{
					return false;
				}
			}

			if (primitive.joints1 >= 0)
			{
				const Accessor& accessor = glTF.accessors[primitive.joints1];

				uint32_t stride = HelperAccess::getStride(accessor);

				VkFormat format = VK_FORMAT_UNDEFINED;
				if (!HelperVulkan::getFormat(format, accessor.componentTypeSize, accessor.componentTypeSigned, accessor.componentTypeInteger, accessor.typeCount, accessor.normalized))
				{
					return false;
				}

				if (!resourceManager.geometryResourceSetAttribute(geometryHandle, glTF.accessors[primitive.joints1].count, glTF.accessors[primitive.joints1].typeCount, "JOINTS_1", format, stride, resourceManager.getBuffer(getBufferHandle(accessor)), HelperAccess::getOffset(accessor), HelperAccess::getRange(accessor)))
				{
					return false;
				}
			}

			if (primitive.weights0 >= 0)
			{
				const Accessor& accessor = glTF.accessors[primitive.weights0];

				uint32_t stride = HelperAccess::getStride(accessor);

				VkFormat format = VK_FORMAT_UNDEFINED;
				if (!HelperVulkan::getFormat(format, accessor.componentTypeSize, accessor.componentTypeSigned, accessor.componentTypeInteger, accessor.typeCount, accessor.normalized))
				{
					return false;
				}

				if (!resourceManager.geometryResourceSetAttribute(geometryHandle, glTF.accessors[primitive.weights0].count, glTF.accessors[primitive.weights0].typeCount, "WEIGHTS_0", format, stride, resourceManager.getBuffer(getBufferHandle(accessor)), HelperAccess::getOffset(accessor), HelperAccess::getRange(accessor)))
				{
					return false;
				}
			}

			if (primitive.weights1 >= 0)
			{
				const Accessor& accessor = glTF.accessors[primitive.weights1];

				uint32_t stride = HelperAccess::getStride(accessor);

				VkFormat format = VK_FORMAT_UNDEFINED;
				if (!HelperVulkan::getFormat(format, accessor.componentTypeSize, accessor.componentTypeSigned, accessor.componentTypeInteger, accessor.typeCount, accessor.normalized))
				{
					return false;
				}

				if (!resourceManager.geometryResourceSetAttribute(geometryHandle, glTF.accessors[primitive.weights1].count, glTF.accessors[primitive.weights1].typeCount, "WEIGHTS_1", format, stride, resourceManager.getBuffer(getBufferHandle(accessor)), HelperAccess::getOffset(accessor), HelperAccess::getRange(accessor)))
				{
					return false;
				}
			}

			//

			resourceManager.geometryResourceFinalize(geometryHandle);

			//

			if (primitive.position < 0)
			{
				return false;
			}

			//

			resourceManager.geometryModelResourceSetGeometryResource(geometryModelHandle, geometryHandle);

			resourceManager.geometryModelResourceSetMaterialResource(geometryModelHandle, materialHandles[primitive.material]);

			//

			if (primitive.targetsCount > 0)
			{
				if (primitive.targetPositionData.size() > 0)
				{
					uint64_t sharedDataHandle = (uint64_t)primitive.targetPositionData.data();

					if (!createSharedDataResource(sizeof(glm::vec3) * primitive.targetPositionData.size(), primitive.targetPositionData.data(), physicalDevice, device, queue, commandPool, useRaytrace))
					{
						return false;
					}

					if (!resourceManager.geometryModelResourceSetTargetData(geometryModelHandle, "POSITION", sharedDataHandle))
					{
						return false;
					}
				}

				if (primitive.targetNormalData.size() > 0)
				{
					uint64_t sharedDataHandle = (uint64_t)primitive.targetNormalData.data();

					if (!createSharedDataResource(sizeof(glm::vec3) * primitive.targetNormalData.size(), primitive.targetNormalData.data(), physicalDevice, device, queue, commandPool, useRaytrace))
					{
						return false;
					}

					if (!resourceManager.geometryModelResourceSetTargetData(geometryModelHandle, "NORMAL", sharedDataHandle))
					{
						return false;
					}
				}

				if (primitive.targetTangentData.size() > 0)
				{
					uint64_t sharedDataHandle = (uint64_t)primitive.targetTangentData.data();

					if (!createSharedDataResource(sizeof(glm::vec3) * primitive.targetTangentData.size(), primitive.targetTangentData.data(), physicalDevice, device, queue, commandPool, useRaytrace))
					{
						return false;
					}

					if (!resourceManager.geometryModelResourceSetTargetData(geometryModelHandle, "TANGENT", sharedDataHandle))
					{
						return false;
					}
				}
			}

			//

			VkIndexType indexType = VK_INDEX_TYPE_NONE_KHR;
			if (primitive.indices >= 0)
			{
				indexType = VK_INDEX_TYPE_UINT8_EXT;
				if (glTF.accessors[primitive.indices].componentTypeSize == 2)
				{
					indexType = VK_INDEX_TYPE_UINT16;
				}
				else if (glTF.accessors[primitive.indices].componentTypeSize == 4)
				{
					indexType = VK_INDEX_TYPE_UINT32;
				}

				if (!resourceManager.geometryModelResourceSetIndices(geometryModelHandle, glTF.accessors[primitive.indices].count, indexType, resourceManager.getBuffer(getBufferHandle(glTF.accessors[primitive.indices])), HelperAccess::getOffset(glTF.accessors[primitive.indices]), HelperAccess::getRange(glTF.accessors[primitive.indices])))
				{
					return false;
				}
			}
			else
			{
				if (!resourceManager.geometryModelResourceSetVertexCount(geometryModelHandle, glTF.accessors[primitive.position].count))
				{
					return false;
				}
			}

			if (!resourceManager.geometryModelResourceFinalize(geometryModelHandle, width, height, renderPass, cullMode, samples, useRaytrace, physicalDevice, device, queue, commandPool))
			{
				return false;
			}

			//

			resourceManager.groupResourceAddGeometryModelResource(groupHandle, geometryModelHandle);
		}

		resourceManager.groupResourceFinalize(groupHandle);
	}

	return true;
}

bool WorldBuilder::buildNodes(const GLTF& glTF, bool useRaytrace)
{
	for (size_t i = 0; i < glTF.nodes.size(); i++)
	{
		const Node& node = glTF.nodes[i];

		nodeHandles.push_back((uint64_t)&node);

		if (node.mesh >= 0)
		{
			resourceManager.instanceResourceSetWorldMatrix(nodeHandles[i], node.worldMatrix);
			resourceManager.instanceResourceSetGroupResource(nodeHandles[i], meshHandles[node.mesh]);
			resourceManager.instanceResourceFinalize(nodeHandles[i]);
		}
	}

	return true;
}

bool WorldBuilder::buildScene(const GLTF& glTF, bool useRaytrace)
{
	if (glTF.defaultScene < glTF.scenes.size())
	{
		for (size_t i = 0; i < glTF.scenes[glTF.defaultScene].nodes.size(); i++)
		{
			resourceManager.worldResourceAddInstanceResource(nodeHandles[glTF.scenes[glTF.defaultScene].nodes[i]]);
		}

		// TODO: Resolve
		if (!finalizeWorld(glTF, physicalDevice, device, queue, commandPool, imageView, useRaytrace))
		{
			return false;
		}

		if (!resourceManager.worldResourceFinalize(device))
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	return true;
}

bool WorldBuilder::build(const GLTF& glTF, const std::string& environment, bool useRaytrace)
{
	glTFHandle = (uint64_t)&glTF;

	resourceManager.lightResourceSetEnvironmentLight(glTFHandle, environment);
	resourceManager.lightResourceFinalize(glTFHandle, physicalDevice, device, queue, commandPool);

	resourceManager.cameraResourceFinalize(glTFHandle);

	resourceManager.worldResourceSetLightResource(glTFHandle);
	resourceManager.worldResourceSetCameraResource(glTFHandle);

	// BufferViews

	if (!buildBufferViews(glTF, useRaytrace))
	{
		return false;
	}

	// Accessors

	if (!buildAccessors(glTF, useRaytrace))
	{
		return false;
	}

	// Textures

	if (!buildTextures(glTF, useRaytrace))
	{
		return false;
	}

	// Materials

	if (!buildMaterials(glTF, useRaytrace))
	{
		return false;
	}

	// Meshes

	if (!buildMeshes(glTF, useRaytrace))
	{
		return false;
	}

	// Nodes

	if (!buildNodes(glTF, useRaytrace))
	{
		return false;
	}

	// Scene

	if (!buildScene(glTF, useRaytrace))
	{
		return false;
	}

	return true;
}


uint64_t WorldBuilder::getBufferHandle(const Accessor& accessor)
{
	uint64_t sharedDataHandle = 0;

	if (accessor.aliasedBuffer.byteLength > 0)
	{
		sharedDataHandle = (uint64_t)&accessor.aliasedBufferView;

		return sharedDataHandle;
	}

	if (accessor.sparse.count >= 1)
	{
		sharedDataHandle = (uint64_t)&accessor.sparse.bufferView;

		return sharedDataHandle;
	}

	if (accessor.pBufferView == nullptr)
	{
		return sharedDataHandle;
	}

	sharedDataHandle = (uint64_t)accessor.pBufferView;

	return sharedDataHandle;
}

bool WorldBuilder::createSharedDataResource(const BufferView& bufferView, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool, bool useRaytrace)
{
	uint64_t sharedDataHandle = (uint64_t)&bufferView;

	VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	if (bufferView.target == 34963) // ELEMENT_ARRAY_BUFFER
	{
		usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	}
	if (useRaytrace)
	{
		usage |= (VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	}
	resourceManager.sharedDataSetUsage(sharedDataHandle, usage);

	resourceManager.sharedDataSetData(sharedDataHandle, bufferView.byteLength, HelperAccess::accessData(bufferView));

	if (!resourceManager.sharedDataResourceFinalize(sharedDataHandle, physicalDevice, device, queue, commandPool))
	{
		return false;
	}

	return true;
}

bool WorldBuilder::createSharedDataResource(VkDeviceSize size, const void* data, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool, bool useRaytrace)
{
	uint64_t sharedDataHandle = (uint64_t)data;

	VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if (useRaytrace)
	{
		usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	}
	resourceManager.sharedDataSetUsage(sharedDataHandle, usage);

	resourceManager.sharedDataSetData(sharedDataHandle, size, data);

	if (!resourceManager.sharedDataResourceFinalize(sharedDataHandle, physicalDevice, device, queue, commandPool))
	{
		return false;
	}

	return true;
}

bool WorldBuilder::finalizeWorld(const GLTF& glTF, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool commandPool, VkImageView imageView, bool useRaytrace)
{
	WorldResource* worldResource = resourceManager.getWorldResource();

	if (!worldResource)
	{
		return false;
	}

	//

	if (useRaytrace)
	{
		TopLevelResourceCreateInfo topLevelResourceCreateInfo = {};

		uint32_t primitiveInstanceID = 0;
		int32_t normalInstanceID = 0;
		int32_t tangentInstanceID = 0;
		int32_t texCoord0InstanceID = 0;

		std::vector<VkDescriptorBufferInfo> descriptorBufferInfoIndices;
		std::vector<VkDescriptorBufferInfo> descriptorBufferInfoPosition;
		std::vector<VkDescriptorBufferInfo> descriptorBufferInfoNormal;
		std::vector<VkDescriptorBufferInfo> descriptorBufferInfoTangent;
		std::vector<VkDescriptorBufferInfo> descriptorBufferInfoTexCoord0;

		for (const Node& node : glTF.nodes)
		{
			InstanceResource* instanceResource = resourceManager.getInstanceResource((uint64_t)&node);

			if (instanceResource->groupHandle > 0)
			{
				GroupResource* groupResource = resourceManager.getGroupResource(instanceResource->groupHandle);

				VkAccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo = {};
				accelerationStructureDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;

				VkTransformMatrixKHR transformMatrix = {
					instanceResource->worldMatrix[0][0], instanceResource->worldMatrix[1][0], instanceResource->worldMatrix[2][0], instanceResource->worldMatrix[3][0],
					instanceResource->worldMatrix[0][1], instanceResource->worldMatrix[1][1], instanceResource->worldMatrix[2][1], instanceResource->worldMatrix[3][1],
					instanceResource->worldMatrix[0][2], instanceResource->worldMatrix[1][2], instanceResource->worldMatrix[2][2], instanceResource->worldMatrix[3][2]
				};

				for (const Primitive& currentPrimitive : glTF.meshes[node.mesh].primitives)
				{
					GeometryModelResource* geometryModelResource = resourceManager.getGeometryModelResource((uint64_t)&currentPrimitive);

					GeometryResource* geometryResource = resourceManager.getGeometryResource(geometryModelResource->geometryHandle);

					//

					accelerationStructureDeviceAddressInfo.accelerationStructure = geometryModelResource->bottomLevelResource.levelResource.accelerationStructureResource.accelerationStructure;
					VkDeviceAddress bottomDeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device, &accelerationStructureDeviceAddressInfo);

					VkAccelerationStructureInstanceKHR accelerationStructureInstance = {};
					accelerationStructureInstance.transform                              = transformMatrix;
					accelerationStructureInstance.instanceCustomIndex                    = primitiveInstanceID;
					accelerationStructureInstance.mask                                   = 0xFF;
					accelerationStructureInstance.instanceShaderBindingTableRecordOffset = 0;
					accelerationStructureInstance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
					accelerationStructureInstance.accelerationStructureReference         = bottomDeviceAddress;

					worldResource->accelerationStructureInstances.push_back(accelerationStructureInstance);

					RaytracePrimitiveUniformBuffer primitiveInformation = {};
					primitiveInformation.materialIndex = currentPrimitive.material;
					primitiveInformation.componentTypeSize = glTF.accessors[currentPrimitive.indices].componentTypeSize;
					primitiveInformation.worldMatrix = instanceResource->worldMatrix;

					if (geometryResource->normalAttributeIndex >= 0)
					{
						primitiveInformation.normalInstanceID = normalInstanceID;

						normalInstanceID++;
					}
					if (geometryResource->tangentAttributeIndex >= 0)
					{
						primitiveInformation.tangentInstanceID = tangentInstanceID;

						tangentInstanceID++;
					}
					if (geometryResource->texCoord0AttributeIndex >= 0)
					{
						primitiveInformation.texCoord0InstanceID = texCoord0InstanceID;

						texCoord0InstanceID++;
					}

					worldResource->instanceResources.push_back(primitiveInformation);

					primitiveInstanceID++;

					//
					// Gather descriptor buffer info.
					//

					if (geometryModelResource->indicesCount > 0)
					{
						VkDescriptorBufferInfo currentDescriptorBufferInfo = {};

						currentDescriptorBufferInfo.buffer = geometryModelResource->indexBuffer;
						currentDescriptorBufferInfo.offset = geometryModelResource->indexOffset;
						currentDescriptorBufferInfo.range = geometryModelResource->indexRange;

						descriptorBufferInfoIndices.push_back(currentDescriptorBufferInfo);
					}

					if (geometryResource->positionAttributeIndex >= 0)
					{
						VkDescriptorBufferInfo currentDescriptorBufferInfo = {};

						currentDescriptorBufferInfo.buffer = geometryResource->vertexBuffers[geometryResource->positionAttributeIndex];
						currentDescriptorBufferInfo.offset = geometryResource->vertexBuffersOffsets[geometryResource->positionAttributeIndex];
						currentDescriptorBufferInfo.range =  geometryResource->vertexBuffersRanges[geometryResource->positionAttributeIndex];

						descriptorBufferInfoPosition.push_back(currentDescriptorBufferInfo);
					}

					if (geometryResource->normalAttributeIndex >= 0)
					{
						VkDescriptorBufferInfo currentDescriptorBufferInfo = {};

						currentDescriptorBufferInfo.buffer = geometryResource->vertexBuffers[geometryResource->normalAttributeIndex];
						currentDescriptorBufferInfo.offset = geometryResource->vertexBuffersOffsets[geometryResource->normalAttributeIndex];
						currentDescriptorBufferInfo.range =  geometryResource->vertexBuffersRanges[geometryResource->normalAttributeIndex];

						descriptorBufferInfoNormal.push_back(currentDescriptorBufferInfo);
					}

					if (geometryResource->tangentAttributeIndex >= 0)
					{
						VkDescriptorBufferInfo currentDescriptorBufferInfo = {};

						currentDescriptorBufferInfo.buffer = geometryResource->vertexBuffers[geometryResource->tangentAttributeIndex];
						currentDescriptorBufferInfo.offset = geometryResource->vertexBuffersOffsets[geometryResource->tangentAttributeIndex];
						currentDescriptorBufferInfo.range =  geometryResource->vertexBuffersRanges[geometryResource->tangentAttributeIndex];

						descriptorBufferInfoTangent.push_back(currentDescriptorBufferInfo);
					}

					if (geometryResource->texCoord0AttributeIndex >= 0)
					{
						VkDescriptorBufferInfo currentDescriptorBufferInfo = {};

						currentDescriptorBufferInfo.buffer = geometryResource->vertexBuffers[geometryResource->texCoord0AttributeIndex];
						currentDescriptorBufferInfo.offset = geometryResource->vertexBuffersOffsets[geometryResource->texCoord0AttributeIndex];
						currentDescriptorBufferInfo.range =  geometryResource->vertexBuffersRanges[geometryResource->texCoord0AttributeIndex];

						descriptorBufferInfoTexCoord0.push_back(currentDescriptorBufferInfo);
					}
				}
			}
		}
		BufferResourceCreateInfo bufferResourceCreateInfo = {};
		bufferResourceCreateInfo.size = sizeof(VkAccelerationStructureInstanceKHR) * worldResource->accelerationStructureInstances.size();
		bufferResourceCreateInfo.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		bufferResourceCreateInfo.memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		if (!VulkanResource::createBufferResource(physicalDevice, device, worldResource->accelerationStructureInstanceBuffer, bufferResourceCreateInfo))
		{
			return false;
		}

		if (!VulkanResource::copyHostToDevice(device, worldResource->accelerationStructureInstanceBuffer, worldResource->accelerationStructureInstances.data(), sizeof(VkAccelerationStructureInstanceKHR) * worldResource->accelerationStructureInstances.size()))
		{
			return false;
		}

		VkBufferDeviceAddressInfo bufferDeviceAddressInfo = {};
		bufferDeviceAddressInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;

		bufferDeviceAddressInfo.buffer = worldResource->accelerationStructureInstanceBuffer.buffer;
		VkDeviceAddress deviceAddress = vkGetBufferDeviceAddressKHR(device, &bufferDeviceAddressInfo);

		//
		// Top level acceleration
		//

		topLevelResourceCreateInfo.deviceAddress = deviceAddress;
		topLevelResourceCreateInfo.primitiveCount = static_cast<uint32_t>(worldResource->accelerationStructureInstances.size());

		topLevelResourceCreateInfo.useHostCommand = false;

		if (!VulkanRaytraceResource::createTopLevelResource(physicalDevice, device, queue, commandPool, worldResource->topLevelResource, topLevelResourceCreateInfo))
		{
			return false;
		}

		//
		// Gather per instance resources.
		//

		StorageBufferResourceCreateInfo storageBufferResourceCreateInfo = {};
		storageBufferResourceCreateInfo.bufferResourceCreateInfo.size = sizeof(RaytracePrimitiveUniformBuffer) * worldResource->instanceResources.size();
		storageBufferResourceCreateInfo.bufferResourceCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		storageBufferResourceCreateInfo.bufferResourceCreateInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		storageBufferResourceCreateInfo.data = worldResource->instanceResources.data();

		if (!VulkanResource::createStorageBufferResource(physicalDevice, device, queue, commandPool, worldResource->instanceResourcesStorageBufferResource, storageBufferResourceCreateInfo))
		{
			return false;
		}

		//
		// Gather material resources.
		//

		std::vector<RaytraceMaterialUniformBuffer> materialBuffers;
		for (const Material& currentMatrial : glTF.materials)
		{
			RaytraceMaterialUniformBuffer uniformBufferRaytrace = {};

			uniformBufferRaytrace.materialUniformBuffer.baseColorFactor = currentMatrial.pbrMetallicRoughness.baseColorFactor;
			uniformBufferRaytrace.materialUniformBuffer.metallicFactor = currentMatrial.pbrMetallicRoughness.metallicFactor;
			uniformBufferRaytrace.materialUniformBuffer.roughnessFactor = currentMatrial.pbrMetallicRoughness.roughnessFactor;
			uniformBufferRaytrace.materialUniformBuffer.normalScale = currentMatrial.normalTexture.scale;
			uniformBufferRaytrace.materialUniformBuffer.occlusionStrength = currentMatrial.occlusionTexture.strength;
			uniformBufferRaytrace.materialUniformBuffer.emissiveFactor = currentMatrial.emissiveFactor;
			uniformBufferRaytrace.materialUniformBuffer.alphaMode = currentMatrial.alphaMode;
			uniformBufferRaytrace.materialUniformBuffer.alphaCutoff = currentMatrial.alphaCutoff;
			uniformBufferRaytrace.materialUniformBuffer.doubleSided = currentMatrial.doubleSided;

			uniformBufferRaytrace.baseColorTexture = currentMatrial.pbrMetallicRoughness.baseColorTexture.index;
			uniformBufferRaytrace.metallicRoughnessTexture = currentMatrial.pbrMetallicRoughness.metallicRoughnessTexture.index;
			uniformBufferRaytrace.normalTexture = currentMatrial.normalTexture.index;
			uniformBufferRaytrace.occlusionTexture = currentMatrial.occlusionTexture.index;
			uniformBufferRaytrace.emissiveTexture = currentMatrial.emissiveTexture.index;

			materialBuffers.push_back(uniformBufferRaytrace);
		}

		storageBufferResourceCreateInfo = {};
		storageBufferResourceCreateInfo.bufferResourceCreateInfo.size = sizeof(RaytraceMaterialUniformBuffer) * glTF.materials.size();
		storageBufferResourceCreateInfo.bufferResourceCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		storageBufferResourceCreateInfo.bufferResourceCreateInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		storageBufferResourceCreateInfo.data = materialBuffers.data();

		if (!VulkanResource::createStorageBufferResource(physicalDevice, device, queue, commandPool, worldResource->materialStorageBufferResource, storageBufferResourceCreateInfo))
		{
			return false;
		}

		//
		// Gather texture resources.
		//

		std::vector<VkDescriptorImageInfo> descriptorImageInfoTextures;
		for (const Texture& currentTexture : glTF.textures)
		{
			TextureDataResource* currentTextureResource = resourceManager.getTextureResource((uint64_t)&currentTexture);

			VkDescriptorImageInfo descriptorImageInfo = {};

			descriptorImageInfo.sampler = currentTextureResource->textureResource.samplerResource.sampler;
			descriptorImageInfo.imageView = currentTextureResource->textureResource.imageViewResource.imageView;
			descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			descriptorImageInfoTextures.push_back(descriptorImageInfo);
		}

		//
		// Create required descriptor sets
		//

		uint32_t binding = 0;

		const uint32_t accelerationBinding = binding++;
		const uint32_t outputBinding = binding++;

		const uint32_t diffuseBinding = binding++;
		const uint32_t specularBinding = binding++;
		const uint32_t lutBinding = binding++;

		const uint32_t texturesBinding = binding++;
		const uint32_t materialStorageBufferResourcesBinding = binding++;
		const uint32_t primitivesBinding = binding++;

		const uint32_t indicesBinding = binding++;
		const uint32_t positionBinding = binding++;
		const uint32_t normalBinding = binding++;
		const uint32_t tangentBinding = binding++;
		const uint32_t texCoord0Binding = binding++;

		std::vector<VkDescriptorSetLayoutBinding> raytraceDescriptorSetLayoutBindings = {};

		VkDescriptorSetLayoutBinding raytraceDescriptorSetLayoutBinding = {};
		raytraceDescriptorSetLayoutBinding.binding         = accelerationBinding;
		raytraceDescriptorSetLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		raytraceDescriptorSetLayoutBinding.descriptorCount = 1;
		raytraceDescriptorSetLayoutBinding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		raytraceDescriptorSetLayoutBindings.push_back(raytraceDescriptorSetLayoutBinding);

		raytraceDescriptorSetLayoutBinding = {};
		raytraceDescriptorSetLayoutBinding.binding         = outputBinding;
		raytraceDescriptorSetLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		raytraceDescriptorSetLayoutBinding.descriptorCount = 1;
		raytraceDescriptorSetLayoutBinding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		raytraceDescriptorSetLayoutBindings.push_back(raytraceDescriptorSetLayoutBinding);

		raytraceDescriptorSetLayoutBinding = {};
		raytraceDescriptorSetLayoutBinding.binding         = diffuseBinding;
		raytraceDescriptorSetLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		raytraceDescriptorSetLayoutBinding.descriptorCount = 1;
		raytraceDescriptorSetLayoutBinding.stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;
		raytraceDescriptorSetLayoutBindings.push_back(raytraceDescriptorSetLayoutBinding);

		raytraceDescriptorSetLayoutBinding = {};
		raytraceDescriptorSetLayoutBinding.binding         = specularBinding;
		raytraceDescriptorSetLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		raytraceDescriptorSetLayoutBinding.descriptorCount = 1;
		raytraceDescriptorSetLayoutBinding.stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;
		raytraceDescriptorSetLayoutBindings.push_back(raytraceDescriptorSetLayoutBinding);

		raytraceDescriptorSetLayoutBinding = {};
		raytraceDescriptorSetLayoutBinding.binding         = lutBinding;
		raytraceDescriptorSetLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		raytraceDescriptorSetLayoutBinding.descriptorCount = 1;
		raytraceDescriptorSetLayoutBinding.stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;
		raytraceDescriptorSetLayoutBindings.push_back(raytraceDescriptorSetLayoutBinding);

		if (descriptorImageInfoTextures.size() > 0)
		{
			raytraceDescriptorSetLayoutBinding = {};
			raytraceDescriptorSetLayoutBinding.binding         = texturesBinding;
			raytraceDescriptorSetLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			raytraceDescriptorSetLayoutBinding.descriptorCount = static_cast<uint32_t>(descriptorImageInfoTextures.size());
			raytraceDescriptorSetLayoutBinding.stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			raytraceDescriptorSetLayoutBindings.push_back(raytraceDescriptorSetLayoutBinding);
		}

		raytraceDescriptorSetLayoutBinding = {};
		raytraceDescriptorSetLayoutBinding.binding         = materialStorageBufferResourcesBinding;
		raytraceDescriptorSetLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		raytraceDescriptorSetLayoutBinding.descriptorCount = 1;
		raytraceDescriptorSetLayoutBinding.stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		raytraceDescriptorSetLayoutBindings.push_back(raytraceDescriptorSetLayoutBinding);

		raytraceDescriptorSetLayoutBinding = {};
		raytraceDescriptorSetLayoutBinding.binding         = primitivesBinding;
		raytraceDescriptorSetLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		raytraceDescriptorSetLayoutBinding.descriptorCount = 1;
		raytraceDescriptorSetLayoutBinding.stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		raytraceDescriptorSetLayoutBindings.push_back(raytraceDescriptorSetLayoutBinding);

		raytraceDescriptorSetLayoutBinding = {};
		raytraceDescriptorSetLayoutBinding.binding         = indicesBinding;
		raytraceDescriptorSetLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		raytraceDescriptorSetLayoutBinding.descriptorCount = static_cast<uint32_t>(descriptorBufferInfoIndices.size());
		raytraceDescriptorSetLayoutBinding.stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		raytraceDescriptorSetLayoutBindings.push_back(raytraceDescriptorSetLayoutBinding);

		raytraceDescriptorSetLayoutBinding = {};
		raytraceDescriptorSetLayoutBinding.binding         = positionBinding;
		raytraceDescriptorSetLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		raytraceDescriptorSetLayoutBinding.descriptorCount = static_cast<uint32_t>(descriptorBufferInfoPosition.size());
		raytraceDescriptorSetLayoutBinding.stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		raytraceDescriptorSetLayoutBindings.push_back(raytraceDescriptorSetLayoutBinding);

		if (descriptorBufferInfoNormal.size() > 0)
		{
			raytraceDescriptorSetLayoutBinding = {};
			raytraceDescriptorSetLayoutBinding.binding         = normalBinding;
			raytraceDescriptorSetLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			raytraceDescriptorSetLayoutBinding.descriptorCount = static_cast<uint32_t>(descriptorBufferInfoNormal.size());
			raytraceDescriptorSetLayoutBinding.stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			raytraceDescriptorSetLayoutBindings.push_back(raytraceDescriptorSetLayoutBinding);
		}

		if (descriptorBufferInfoTangent.size() > 0)
		{
			raytraceDescriptorSetLayoutBinding = {};
			raytraceDescriptorSetLayoutBinding.binding         = tangentBinding;
			raytraceDescriptorSetLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			raytraceDescriptorSetLayoutBinding.descriptorCount = static_cast<uint32_t>(descriptorBufferInfoTangent.size());
			raytraceDescriptorSetLayoutBinding.stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			raytraceDescriptorSetLayoutBindings.push_back(raytraceDescriptorSetLayoutBinding);
		}

		if (descriptorBufferInfoTexCoord0.size() > 0)
		{
			raytraceDescriptorSetLayoutBinding = {};
			raytraceDescriptorSetLayoutBinding.binding         = texCoord0Binding;
			raytraceDescriptorSetLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			raytraceDescriptorSetLayoutBinding.descriptorCount = static_cast<uint32_t>(descriptorBufferInfoTexCoord0.size());
			raytraceDescriptorSetLayoutBinding.stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			raytraceDescriptorSetLayoutBindings.push_back(raytraceDescriptorSetLayoutBinding);
		}

		VkDescriptorSetLayoutCreateInfo raytraceDescriptorSetLayoutCreateInfo = {};
		raytraceDescriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		raytraceDescriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(raytraceDescriptorSetLayoutBindings.size());
		raytraceDescriptorSetLayoutCreateInfo.pBindings = raytraceDescriptorSetLayoutBindings.data();

		VkResult result = vkCreateDescriptorSetLayout(device, &raytraceDescriptorSetLayoutCreateInfo, nullptr, &worldResource->raytraceDescriptorSetLayout);
		if (result != VK_SUCCESS)
		{
			Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, result);

			return false;
		}

		std::vector<VkDescriptorPoolSize> raytraceDescriptorPoolSizes;

		VkDescriptorPoolSize raytraceDescriptorPoolSize = {};
		raytraceDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		raytraceDescriptorPoolSize.descriptorCount = 1;
		raytraceDescriptorPoolSizes.push_back(raytraceDescriptorPoolSize);

		raytraceDescriptorPoolSize = {};
		raytraceDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		raytraceDescriptorPoolSize.descriptorCount = 1;
		raytraceDescriptorPoolSizes.push_back(raytraceDescriptorPoolSize);

		raytraceDescriptorPoolSize = {};
		raytraceDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		raytraceDescriptorPoolSize.descriptorCount = 1;
		raytraceDescriptorPoolSizes.push_back(raytraceDescriptorPoolSize);

		raytraceDescriptorPoolSize = {};
		raytraceDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		raytraceDescriptorPoolSize.descriptorCount = 1;
		raytraceDescriptorPoolSizes.push_back(raytraceDescriptorPoolSize);

		raytraceDescriptorPoolSize = {};
		raytraceDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		raytraceDescriptorPoolSize.descriptorCount = 1;
		raytraceDescriptorPoolSizes.push_back(raytraceDescriptorPoolSize);

		if (descriptorImageInfoTextures.size() > 0)
		{
			raytraceDescriptorPoolSize = {};
			raytraceDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			raytraceDescriptorPoolSize.descriptorCount = static_cast<uint32_t>(descriptorImageInfoTextures.size());
			raytraceDescriptorPoolSizes.push_back(raytraceDescriptorPoolSize);
		}

		raytraceDescriptorPoolSize = {};
		raytraceDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		raytraceDescriptorPoolSize.descriptorCount = 1;
		raytraceDescriptorPoolSizes.push_back(raytraceDescriptorPoolSize);

		raytraceDescriptorPoolSize = {};
		raytraceDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		raytraceDescriptorPoolSize.descriptorCount = 1;
		raytraceDescriptorPoolSizes.push_back(raytraceDescriptorPoolSize);

		raytraceDescriptorPoolSize = {};
		raytraceDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		raytraceDescriptorPoolSize.descriptorCount = static_cast<uint32_t>(descriptorBufferInfoIndices.size());
		raytraceDescriptorPoolSizes.push_back(raytraceDescriptorPoolSize);

		raytraceDescriptorPoolSize = {};
		raytraceDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		raytraceDescriptorPoolSize.descriptorCount = static_cast<uint32_t>(descriptorBufferInfoPosition.size());
		raytraceDescriptorPoolSizes.push_back(raytraceDescriptorPoolSize);

		if (descriptorBufferInfoNormal.size() > 0)
		{
			raytraceDescriptorPoolSize = {};
			raytraceDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			raytraceDescriptorPoolSize.descriptorCount = static_cast<uint32_t>(descriptorBufferInfoNormal.size());
			raytraceDescriptorPoolSizes.push_back(raytraceDescriptorPoolSize);
		}

		if (descriptorBufferInfoTangent.size() > 0)
		{
			raytraceDescriptorPoolSize = {};
			raytraceDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			raytraceDescriptorPoolSize.descriptorCount = static_cast<uint32_t>(descriptorBufferInfoTangent.size());
			raytraceDescriptorPoolSizes.push_back(raytraceDescriptorPoolSize);
		}

		if (descriptorBufferInfoTexCoord0.size() > 0)
		{
			raytraceDescriptorPoolSize = {};
			raytraceDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			raytraceDescriptorPoolSize.descriptorCount = static_cast<uint32_t>(descriptorBufferInfoTexCoord0.size());
			raytraceDescriptorPoolSizes.push_back(raytraceDescriptorPoolSize);
		}

		VkDescriptorPoolCreateInfo raytraceDescriptorPoolCreateInfo = {};
		raytraceDescriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		raytraceDescriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(raytraceDescriptorPoolSizes.size());
		raytraceDescriptorPoolCreateInfo.pPoolSizes = raytraceDescriptorPoolSizes.data();
		raytraceDescriptorPoolCreateInfo.maxSets = 1;

		result = vkCreateDescriptorPool(device, &raytraceDescriptorPoolCreateInfo, nullptr, &worldResource->raytraceDescriptorPool);
		if (result != VK_SUCCESS)
		{
			Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, result);

			return false;
		}

		VkDescriptorSetAllocateInfo raytraceDescriptorSetAllocateInfo = {};
		raytraceDescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		raytraceDescriptorSetAllocateInfo.descriptorPool = worldResource->raytraceDescriptorPool;
		raytraceDescriptorSetAllocateInfo.descriptorSetCount = 1;
		raytraceDescriptorSetAllocateInfo.pSetLayouts = &worldResource->raytraceDescriptorSetLayout;

		result = vkAllocateDescriptorSets(device, &raytraceDescriptorSetAllocateInfo, &worldResource->raytraceDescriptorSet);
		if (result != VK_SUCCESS)
		{
			Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, result);

			return false;
		}

		//
		//

		VkWriteDescriptorSetAccelerationStructureKHR writeDescriptorSetAccelerationStructure = {};
		writeDescriptorSetAccelerationStructure.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		writeDescriptorSetAccelerationStructure.accelerationStructureCount = 1;
		writeDescriptorSetAccelerationStructure.pAccelerationStructures    = &worldResource->topLevelResource.levelResource.accelerationStructureResource.accelerationStructure;

		VkDescriptorImageInfo descriptorImageInfo = {};
		descriptorImageInfo.sampler = VK_NULL_HANDLE;
		descriptorImageInfo.imageView = imageView;
		descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkDescriptorBufferInfo descriptorBufferInfo = {};
		descriptorBufferInfo.buffer = worldResource->materialStorageBufferResource.bufferResource.buffer;
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range = sizeof(RaytraceMaterialUniformBuffer) * glTF.materials.size();

		VkDescriptorBufferInfo descriptorBufferInfo2 = {};
		descriptorBufferInfo2.buffer = worldResource->instanceResourcesStorageBufferResource.bufferResource.buffer;
		descriptorBufferInfo2.offset = 0;
		descriptorBufferInfo2.range = sizeof(RaytracePrimitiveUniformBuffer) * worldResource->instanceResources.size();

		LightResource* lightResource = resourceManager.getLightResource(worldResource->lightHandle);

		VkDescriptorImageInfo descriptorImageInfoDiffuse = {};
		descriptorImageInfoDiffuse.sampler = lightResource->diffuse.samplerResource.sampler;
		descriptorImageInfoDiffuse.imageView = lightResource->diffuse.imageViewResource.imageView;
		descriptorImageInfoDiffuse.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkDescriptorImageInfo descriptorImageInfoSpecular = {};
		descriptorImageInfoSpecular.sampler = lightResource->specular.samplerResource.sampler;
		descriptorImageInfoSpecular.imageView = lightResource->specular.imageViewResource.imageView;
		descriptorImageInfoSpecular.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkDescriptorImageInfo descriptorImageInfoLut = {};
		descriptorImageInfoLut.sampler = lightResource->lut.samplerResource.sampler;
		descriptorImageInfoLut.imageView = lightResource->lut.imageViewResource.imageView;
		descriptorImageInfoLut.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		std::vector<VkWriteDescriptorSet> writeDescriptorSets;

		VkWriteDescriptorSet writeDescriptorSet = {};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet          = worldResource->raytraceDescriptorSet;
		writeDescriptorSet.dstBinding      = accelerationBinding;
		writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.pNext           = &writeDescriptorSetAccelerationStructure;	// Chained
		writeDescriptorSets.push_back(writeDescriptorSet);

		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = worldResource->raytraceDescriptorSet;
		writeDescriptorSet.dstBinding = outputBinding;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.pImageInfo = &descriptorImageInfo;
		writeDescriptorSets.push_back(writeDescriptorSet);

		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = worldResource->raytraceDescriptorSet;
		writeDescriptorSet.dstBinding = diffuseBinding;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.pImageInfo = &descriptorImageInfoDiffuse;
		writeDescriptorSets.push_back(writeDescriptorSet);

		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = worldResource->raytraceDescriptorSet;
		writeDescriptorSet.dstBinding = specularBinding;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.pImageInfo = &descriptorImageInfoSpecular;
		writeDescriptorSets.push_back(writeDescriptorSet);

		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = worldResource->raytraceDescriptorSet;
		writeDescriptorSet.dstBinding = lutBinding;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.pImageInfo = &descriptorImageInfoLut;
		writeDescriptorSets.push_back(writeDescriptorSet);

		if (descriptorImageInfoTextures.size() > 0)
		{
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = worldResource->raytraceDescriptorSet;
			writeDescriptorSet.dstBinding = texturesBinding;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSet.descriptorCount = static_cast<uint32_t>(descriptorImageInfoTextures.size());
			writeDescriptorSet.pImageInfo = descriptorImageInfoTextures.data();
			writeDescriptorSets.push_back(writeDescriptorSet);
		}

		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = worldResource->raytraceDescriptorSet;
		writeDescriptorSet.dstBinding = materialStorageBufferResourcesBinding;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
		writeDescriptorSets.push_back(writeDescriptorSet);

		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = worldResource->raytraceDescriptorSet;
		writeDescriptorSet.dstBinding = primitivesBinding;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.pBufferInfo = &descriptorBufferInfo2;
		writeDescriptorSets.push_back(writeDescriptorSet);

		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = worldResource->raytraceDescriptorSet;
		writeDescriptorSet.dstBinding = indicesBinding;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writeDescriptorSet.descriptorCount = static_cast<uint32_t>(descriptorBufferInfoIndices.size());
		writeDescriptorSet.pBufferInfo = descriptorBufferInfoIndices.data();
		writeDescriptorSets.push_back(writeDescriptorSet);

		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = worldResource->raytraceDescriptorSet;
		writeDescriptorSet.dstBinding = positionBinding;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writeDescriptorSet.descriptorCount = static_cast<uint32_t>(descriptorBufferInfoPosition.size());
		writeDescriptorSet.pBufferInfo = descriptorBufferInfoPosition.data();
		writeDescriptorSets.push_back(writeDescriptorSet);

		if (descriptorBufferInfoNormal.size() > 0)
		{
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = worldResource->raytraceDescriptorSet;
			writeDescriptorSet.dstBinding = normalBinding;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeDescriptorSet.descriptorCount = static_cast<uint32_t>(descriptorBufferInfoNormal.size());
			writeDescriptorSet.pBufferInfo = descriptorBufferInfoNormal.data();
			writeDescriptorSets.push_back(writeDescriptorSet);
		}

		if (descriptorBufferInfoTangent.size() > 0)
		{
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = worldResource->raytraceDescriptorSet;
			writeDescriptorSet.dstBinding = tangentBinding;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeDescriptorSet.descriptorCount = static_cast<uint32_t>(descriptorBufferInfoTangent.size());
			writeDescriptorSet.pBufferInfo = descriptorBufferInfoTangent.data();
			writeDescriptorSets.push_back(writeDescriptorSet);
		}

		if (descriptorBufferInfoTexCoord0.size() > 0)
		{
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = worldResource->raytraceDescriptorSet;
			writeDescriptorSet.dstBinding = texCoord0Binding;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeDescriptorSet.descriptorCount = static_cast<uint32_t>(descriptorBufferInfoTexCoord0.size());
			writeDescriptorSet.pBufferInfo = descriptorBufferInfoTexCoord0.data();
			writeDescriptorSets.push_back(writeDescriptorSet);
		}

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

		//
		// Creating ray tracing raytracePipeline.
		//

		VkPushConstantRange pushConstantRange = {};
	    pushConstantRange.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	    pushConstantRange.offset = 0;
	    pushConstantRange.size = sizeof(RaytraceUniformPushConstant);

		VkPipelineLayoutCreateInfo raytracePipelineLayoutCreateInfo = {};
		raytracePipelineLayoutCreateInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		raytracePipelineLayoutCreateInfo.setLayoutCount = 1;
		raytracePipelineLayoutCreateInfo.pSetLayouts    = &worldResource->raytraceDescriptorSetLayout;
		raytracePipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		raytracePipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

		result = vkCreatePipelineLayout(device, &raytracePipelineLayoutCreateInfo, nullptr, &worldResource->raytracePipelineLayout);
		if (result != VK_SUCCESS)
		{
			Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, result);

			return false;
		}

		//
		// Build the required raytracing shaders.
		//

		std::map<std::string, std::string> macros;

		macros["ACCELERATION_BINDING"] = std::to_string(accelerationBinding);
		macros["OUTPUT_BINDING"] = std::to_string(outputBinding);

		macros["DIFFUSE_BINDING"] = std::to_string(diffuseBinding);
		macros["SPECULAR_BINDING"] = std::to_string(specularBinding);
		macros["LUT_BINDING"] = std::to_string(lutBinding);

		macros["TEXTURES_BINDING"] = std::to_string(texturesBinding);
		macros["MATERIALS_BINDING"] = std::to_string(materialStorageBufferResourcesBinding);
		macros["PRIMITIVES_BINDING"] = std::to_string(primitivesBinding);

		macros["INDICES_BINDING"] = std::to_string(indicesBinding);
		macros["POSITION_BINDING"] = std::to_string(positionBinding);
		macros["NORMAL_BINDING"] = std::to_string(normalBinding);
		macros["TANGENT_BINDING"] = std::to_string(tangentBinding);
		macros["TEXCOORD0_BINDING"] = std::to_string(texCoord0Binding);

		if (descriptorImageInfoTextures.size() > 0)
		{
			macros["HAS_TEXTURES"] = "";
		}

		if (descriptorBufferInfoNormal.size() > 0)
		{
			macros["NORMAL_VEC3"] = "";
		}
		if (descriptorBufferInfoTangent.size() > 0)
		{
			macros["TANGENT_VEC4"] = "";
		}
		if (descriptorBufferInfoTexCoord0.size() > 0)
		{
			macros["TEXCOORD0_VEC2"] = "";
		}

		std::string rayGenShaderSource = "";
		if (!FileIO::open(rayGenShaderSource, "../Resources/shaders/gltf.rgen"))
		{
			return false;
		}

		std::vector<uint32_t> rayGenShaderCode;
		if (!Compiler::buildShader(rayGenShaderCode, rayGenShaderSource, macros, shaderc_raygen_shader))
		{
			return false;
		}

		if (!VulkanResource::createShaderModule(worldResource->rayGenShaderModule, device, rayGenShaderCode))
		{
			return false;
		}

		//

		std::string missShaderSource = "";
		if (!FileIO::open(missShaderSource, "../Resources/shaders/gltf.rmiss"))
		{
			return false;
		}

		std::vector<uint32_t> missShaderCode;
		if (!Compiler::buildShader(missShaderCode, missShaderSource, macros, shaderc_miss_shader))
		{
			return false;
		}

		if (!VulkanResource::createShaderModule(worldResource->missShaderModule, device, missShaderCode))
		{
			return false;
		}

		//

		std::string closestHitShaderSource = "";
		if (!FileIO::open(closestHitShaderSource, "../Resources/shaders/gltf.rchit"))
		{
			return false;
		}

		std::vector<uint32_t> closestHitShaderCode;
		if (!Compiler::buildShader(closestHitShaderCode, closestHitShaderSource, macros, shaderc_closesthit_shader))
		{
			return false;
		}

		if (!VulkanResource::createShaderModule(worldResource->closestHitShaderModule, device, closestHitShaderCode))
		{
			return false;
		}

		//
		//

		std::vector<VkPipelineShaderStageCreateInfo> raytracePipelineShaderStageCreateInfos;
		raytracePipelineShaderStageCreateInfos.resize(3);

		raytracePipelineShaderStageCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		raytracePipelineShaderStageCreateInfos[0].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		raytracePipelineShaderStageCreateInfos[0].module = worldResource->rayGenShaderModule;
		raytracePipelineShaderStageCreateInfos[0].pName = "main";

		raytracePipelineShaderStageCreateInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		raytracePipelineShaderStageCreateInfos[1].stage = VK_SHADER_STAGE_MISS_BIT_KHR;
		raytracePipelineShaderStageCreateInfos[1].module = worldResource->missShaderModule;
		raytracePipelineShaderStageCreateInfos[1].pName = "main";

		raytracePipelineShaderStageCreateInfos[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		raytracePipelineShaderStageCreateInfos[2].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		raytracePipelineShaderStageCreateInfos[2].module = worldResource->closestHitShaderModule;
		raytracePipelineShaderStageCreateInfos[2].pName = "main";

		//

		std::vector<VkRayTracingShaderGroupCreateInfoKHR> rayTracingShaderGroupCreateInfos = {};
		rayTracingShaderGroupCreateInfos.resize(3);

		rayTracingShaderGroupCreateInfos[0] = {};
		rayTracingShaderGroupCreateInfos[0].sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		rayTracingShaderGroupCreateInfos[0].type 			   = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		rayTracingShaderGroupCreateInfos[0].generalShader      = 0;
		rayTracingShaderGroupCreateInfos[0].closestHitShader   = VK_SHADER_UNUSED_KHR;
		rayTracingShaderGroupCreateInfos[0].anyHitShader       = VK_SHADER_UNUSED_KHR;
		rayTracingShaderGroupCreateInfos[0].intersectionShader = VK_SHADER_UNUSED_KHR;

		rayTracingShaderGroupCreateInfos[1] = {};
		rayTracingShaderGroupCreateInfos[1].sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		rayTracingShaderGroupCreateInfos[1].type 			   = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		rayTracingShaderGroupCreateInfos[1].generalShader      = 1;
		rayTracingShaderGroupCreateInfos[1].closestHitShader   = VK_SHADER_UNUSED_KHR;
		rayTracingShaderGroupCreateInfos[1].anyHitShader       = VK_SHADER_UNUSED_KHR;
		rayTracingShaderGroupCreateInfos[1].intersectionShader = VK_SHADER_UNUSED_KHR;

		rayTracingShaderGroupCreateInfos[2] = {};
		rayTracingShaderGroupCreateInfos[2].sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		rayTracingShaderGroupCreateInfos[2].type 			   = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		rayTracingShaderGroupCreateInfos[2].generalShader      = VK_SHADER_UNUSED_KHR;
		rayTracingShaderGroupCreateInfos[2].closestHitShader   = 2;
		rayTracingShaderGroupCreateInfos[2].anyHitShader       = VK_SHADER_UNUSED_KHR;
		rayTracingShaderGroupCreateInfos[2].intersectionShader = VK_SHADER_UNUSED_KHR;

		VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo = {};
		rayTracingPipelineCreateInfo.sType             = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		rayTracingPipelineCreateInfo.stageCount        = static_cast<uint32_t>(raytracePipelineShaderStageCreateInfos.size());
		rayTracingPipelineCreateInfo.pStages           = raytracePipelineShaderStageCreateInfos.data();
		rayTracingPipelineCreateInfo.groupCount        = static_cast<uint32_t>(rayTracingShaderGroupCreateInfos.size());
		rayTracingPipelineCreateInfo.pGroups           = rayTracingShaderGroupCreateInfos.data();
		rayTracingPipelineCreateInfo.maxRecursionDepth = 2;
		rayTracingPipelineCreateInfo.layout            = worldResource->raytracePipelineLayout;
		rayTracingPipelineCreateInfo.libraries.sType   = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;

		result = vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, 1, &rayTracingPipelineCreateInfo, nullptr, &worldResource->raytracePipeline);
		if (result != VK_SUCCESS)
		{
			Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, result);

			return false;
		}

		//
		// Build up shader binding table.
		//

		VkPhysicalDeviceProperties2 physicalDeviceProperties2 = {};
		physicalDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		VkPhysicalDeviceRayTracingPropertiesKHR physicalDeviceRayTracingProperties = {};
		physicalDeviceRayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_KHR;

		physicalDeviceProperties2.pNext = &physicalDeviceRayTracingProperties;
		vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties2);

		bufferResourceCreateInfo = {};
		bufferResourceCreateInfo.size = physicalDeviceRayTracingProperties.shaderGroupHandleSize * rayTracingShaderGroupCreateInfos.size();
		bufferResourceCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR;
		bufferResourceCreateInfo.memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		if (!VulkanResource::createBufferResource(physicalDevice, device, worldResource->shaderBindingBufferResource, bufferResourceCreateInfo))
		{
			return false;
		}
		worldResource->size = physicalDeviceRayTracingProperties.shaderGroupHandleSize;

		std::vector<uint8_t> rayTracingShaderGroupHandles(bufferResourceCreateInfo.size);

		result = vkGetRayTracingShaderGroupHandlesKHR(device, worldResource->raytracePipeline, 0, static_cast<uint32_t>(rayTracingShaderGroupCreateInfos.size()), rayTracingShaderGroupHandles.size(), rayTracingShaderGroupHandles.data());
		if (result != VK_SUCCESS)
		{
			Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, result);

			return false;
		}

		if (!VulkanResource::copyHostToDevice(device, worldResource->shaderBindingBufferResource, rayTracingShaderGroupHandles.data(), rayTracingShaderGroupHandles.size()))
		{
			return false;
		}
	}

	return true;
}

void WorldBuilder::terminate(VkDevice device)
{
	resourceManager.terminate(device);
}
