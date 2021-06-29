//
// Created by nineball on 5/29/21.
//
//
#include "Helper/Device.h"
#include "Helper/Display.h"
#include "Renderer.h"
#include "chrono"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>
#include "Core/NEShared.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

    new (&display) NEDisplay(900, 600, "gooba googa", false, 0);;
    new (&device) NEDevice(instance);

    display = &vkContext.display;
    device = &vkContext.device;

    display->recreateSwapchain();
    device->createBuffers(dispvkBootstrap librarylay->framebufferSize());
    pipeline = device->pipeline();
}

NERenderer::~NERenderer() {

}

bool NERenderer::renderFrame() {
    uint32_t i = 0;

    for (auto display : displays) {
        if (!display->shouldExit()) {
            display->startFrame();
            drawFrame(i);
            display->endFrame();
            i++;
        }
    }

}

void NERenderer::drawFrame(uint32_t display) {

}

void NERenderer::drawObjects(uint32_t display, VkCommandBuffer cmd) {
    uint32_t i = 0;
    for (auto entityID : mEntities[display]) {
        
        VkRenderable entity = coordinator.GetComponent<VkRenderable>(entityID);

        if (boundPipeline != entity.pipeline) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelines[entity.pipeline]);

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, 1, objectdescriptorset);

            if (entity.textureImage != VK_NULL_HANDLE) {

            }
        }

        vkCmdDraw(cmd, 4, 1, 0, i);
        i++;
    }

}


void NERenderer::loadModel(VkDeviceMemory &vertexMemory, VkBuffer &vertexBuffer, VkDeviceMemory &indexMemory, VkBuffer &indexBuffer) {
    device->createVertexBuffers(vertexMemory, vertexBuffer, vertices);
    device->createIndexBuffers(indexMemory, indexBuffer, indices);
}

VkRenderable NERenderer::loadEntityObjects(std::string texture, std::string model) {
    VkRenderable entity;
    loadModel(entity.vertexBufferMemory, entity.vertexBuffer, entity.indexBufferMemory, entity.indexBuffer);
    loadTexture(entity.textureImage, entity.textureImageView, entity.textureImageMemory);
}


NERenderer::loadTexture(VkImage &textureImage, VkImageView &textureImageView, VkDeviceMemory &textureImageMemory) {
    createTextureImage(textureImage, textureImageMemory);
    createTextureImageView(textureImage, textureImageView);
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

    ubo.proj = glm::perspective(glm::radians(45.0f), display->extent().width / (float) display->extent().width, 0.1f,
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
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, device->GPU());

    if (vkAllocateMemory(*device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(*device, image, imageMemory, 0);

}

void NERenderer::createTextureImageView(VkImage &image, VkImageView &imageView) {
    imageView = createImageView(image, VK_FORMAT_R8G8B8A8_SRGB, *device);
}

void NERenderer::createPipeline() {
    pipelines[pipelineSize] = NEPipeline();
}