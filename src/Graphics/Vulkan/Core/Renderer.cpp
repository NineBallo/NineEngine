//
// Created by nineball on 5/29/21.
//
#include "Renderer.h"
#include "chrono"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>

void VkRenderer::draw() {

};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
};

//const std::vector<NEVK::Vertex> vertices = {
//        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
//        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
//        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
//   //     {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
//
//   //   {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
//   //   {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
//   //   {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
//   //   {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
//};

namespace NEVK {
    NERenderer::NERenderer() {
        instance = std::make_shared<NEInstance>(true);
        device = std::make_shared<NEDevice>(instance);
        //device->createWindow();
        swapchain = device->getWindow()->getSwapchain();
        pipeline = device->getPipeline();



        coordinator.RegisterSystem<VkRenderer>();
        Signature signature;
        signature.set(coordinator.GetComponentType<Transform>());
      //  signature.set(coordinator.GetComponentType<Forces>());
        signature.set(coordinator.GetComponentType<VkRenderable>());
        coordinator.SetSystemSignature<VkRenderer>(signature);

        size_t entity = coordinator.CreateEntity();
        coordinator.AddComponent(
                entity,
                Transform {
                    .position = glm::vec3(0, 0, 0),
                    .rotation = glm::vec3(0, 0, 0),
                    .scale = 1
                });

    }
    NERenderer::~NERenderer() {

    }

    void NERenderer::renderFrame() {
        swapchain->startFrame();
        drawFrame();
        swapchain->endFrame();
    }

    void NERenderer::drawFrame() {
        swapchain->submitCommandBuffer(commandBuffers[swapchain->getFrame()]);
    }

    void NERenderer::createCommandBuffers(size_t entity) {
        auto& VKEntity = coordinator.GetComponent<VkRenderable>(entity);

        commandBuffers.resize(swapchain->framebuffers.size());

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

        if (vkAllocateCommandBuffers(*device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffers!");
        }

        for (size_t i = 0; i < commandBuffers.size(); i++) {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // Optional
            beginInfo.pInheritanceInfo = nullptr; // Optional

            if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("Failed to begin recording command buffer!");
            }

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = *device->getRenderpass();
            renderPassInfo.framebuffer = swapchain->framebuffers[i];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = device->getWindow()->getExtent();

            VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);

            VkBuffer vertexBuffers[] = {VKEntity.vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

            vkCmdBindIndexBuffer(commandBuffers[i], VKEntity.indexBuffer, 0, VK_INDEX_TYPE_UINT16);

            vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipelineLayout(), 0, 1, &pipeline->getDescriptorSets[i], 0, nullptr);

            vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

            vkCmdEndRenderPass(commandBuffers[i]);

            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }
    }

    void NERenderer::updateUniformBuffers(size_t entity, int currentImage) {
            auto& VKEntity = coordinator.GetComponent<VkRenderable>(entity);

            static auto startTime = std::chrono::high_resolution_clock::now();

            auto currentTime = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();


            UniformBufferObject ubo{};

            //rotate 90degrees/sec
            ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

            //change camera position to 45degrees above
            ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

            ubo.proj = glm::perspective(glm::radians(45.0f), device->getWindow()->getExtent().width / (float) device->getWindow()->getExtent().height, 0.1f, 10.0f);
            //was made for opengl, need to flip scaling factor of y
            ubo.proj[1][1] *= -1;

            void* data;
            vkMapMemory(*device, VKEntity.uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
            memcpy(data, &ubo, sizeof(ubo));
            vkUnmapMemory(*device, VKEntity.uniformBuffersMemory[currentImage]);
    }
}

