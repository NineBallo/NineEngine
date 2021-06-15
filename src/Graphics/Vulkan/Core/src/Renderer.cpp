//
// Created by nineball on 5/29/21.
//
//
#include "../../Helper/Device.h"
#include "../../Helper/Display.h"
#include "../Renderer.h"
#include "chrono"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>
#include "../NEShared.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

VKContext vkContext;

const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
};

const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};


NERenderer::NERenderer() {
    std::cout << "renderer starting...\n";
    instance = std::make_shared<NEInstance>(true);

    vkContext.displays.emplace_back(NEDisplay(900, 600, "gooba googa", false, 0));
    display = &vkContext.displays[0];

    vkContext.devices.emplace_back(NEDevice(instance));
    device = &vkContext.devices[0];

    display->recreateSwapchain();
    device->createBuffers();
    pipeline = device->getPipeline();


    size_t entity = coordinator.CreateEntity();
    coordinator.AddComponent(
            entity,
            Transform{
                    .position = glm::vec3(0, 0, 0),
                    .rotation = glm::vec3(0, 0, 0),
                    .scale = 1
            });
    coordinator.AddComponent(
            entity,
            Forces{
                    .velocity = glm::vec3(0, 0, 0),
                    .acceleration = glm::vec3(0, 0, 0)
            });
    coordinator.AddComponent(
            entity,
            createEntity()
    );
}

NERenderer::~NERenderer() {

}

VkRenderable NERenderer::createEntity() {
    VkRenderable vkEntity;
    loadTexture(vkEntity.textureImage, vkContext.textureImageView, vkEntity.textureImageMemory);
    loadModel(vkEntity);
    createCommandBuffers(vkEntity);
    return vkEntity;
}

bool NERenderer::renderFrame() {
    if (!display->shouldExit()) {
        vkContext.displays[0].startFrame();
        drawFrame();
        vkContext.displays[0].endFrame();
        return true;
    } else {
        return false;
    }
}

void NERenderer::drawFrame() {
    for (auto const &entity : mEntities) {
        auto &VKEntity = coordinator.GetComponent<VkRenderable>(entity);
        if (VKEntity.hidden) {
            device->submitCommandBuffer(VKEntity.commandBuffers[vkContext.displays[0].getFrame()]);
            updateUniformBuffers(entity, vkContext.displays[0].getFramebufferSize());
        } else {
            loadTexture(VKEntity.textureImage, vkContext.textureImageView, VKEntity.textureImageMemory);
            loadModel(VKEntity);
            createCommandBuffers(VKEntity);
            VKEntity.hidden = false;
        }
    }
}

void NERenderer::loadModel(VkRenderable &VKEntity) {
    device->createVertexBuffers(VKEntity.vertexBufferMemory, VKEntity.vertexBuffer, vertices);
    device->createIndexBuffers(VKEntity.indexBufferMemory, VKEntity.indexBuffer, indices);
}

void
NERenderer::loadTexture(VkImage &textureImage, VkImageView &textureImageView, VkDeviceMemory &textureImageMemory) {
    createTextureImage(textureImage, textureImageMemory);
    createTextureImageView(textureImage, textureImageView);
}



void NERenderer::createCommandBuffers(VkRenderable &VKEntity) {

    VKEntity.commandBuffers.resize(display->getFramebufferSize());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vkContext.devices[0].getCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) VKEntity.commandBuffers.size();

    if (vkAllocateCommandBuffers(*device, &allocInfo, VKEntity.commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }

    for (size_t i = 0; i < VKEntity.commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(VKEntity.commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = vkContext.renderpass;
        renderPassInfo.framebuffer = display->getFrameBuffers()[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = vkContext.displays[0].getExtent();

        VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(VKEntity.commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(VKEntity.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);

        VkBuffer vertexBuffers[] = {VKEntity.vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(VKEntity.commandBuffers[i], 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(VKEntity.commandBuffers[i], VKEntity.indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdBindDescriptorSets(VKEntity.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipeline->getPipelineLayout(), 0, 1, &pipeline->getDescriptorSets()[i], 0, nullptr);

        vkCmdDrawIndexed(VKEntity.commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(VKEntity.commandBuffers[i]);

        if (vkEndCommandBuffer(VKEntity.commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

void NERenderer::updateUniformBuffers(size_t entity, int currentImage) {
    auto &VKEntity = coordinator.GetComponent<VkRenderable>(entity);

    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();


    UniformBufferObject ubo{};

    //rotate 90degrees/sec
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    //change camera position to 45degrees above
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    ubo.proj = glm::perspective(glm::radians(45.0f), display->getExtent().width / (float) display->getExtent().width, 0.1f,
                                10.0f);
    //was made for opengl, need to flip scaling factor of y
    ubo.proj[1][1] *= -1;

    void *data;
    vkMapMemory(*device, vkContext.uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(*device, vkContext.uniformBuffersMemory[currentImage]);
}
void NERenderer::createTextureImage(VkImage& textureImage, VkDeviceMemory& textureImageMemory) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("Textures/image0.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("Failed to load texture image!");
    }
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    device->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(*device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(*device, stagingBufferMemory);

    stbi_image_free(pixels);



    createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                textureImage, textureImageMemory);


    device->transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    device->copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth),
                              static_cast<uint32_t>(texHeight));

    device->transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(*device, stagingBuffer, nullptr);
    vkFreeMemory(*device, stagingBufferMemory, nullptr);
}

void NERenderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                             VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;

    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0; // Optional

    if (vkCreateImage(*device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(*device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, device->getGPU());

    if (vkAllocateMemory(*device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(*device, image, imageMemory, 0);

}

void NERenderer::createTextureImageView(VkImage &image, VkImageView &imageView) {
    imageView = createImageView(image, VK_FORMAT_R8G8B8A8_SRGB, *device);
}
