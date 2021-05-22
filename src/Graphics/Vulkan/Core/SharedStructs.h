//
// Created by nineball on 5/3/21.
//

#ifndef NINEENGINE_SHAREDSTRUCTS_H
#define NINEENGINE_SHAREDSTRUCTS_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <array>
#include <set>
#include <optional>
#include <glm/glm.hpp>
#include <chrono>
#include <string>

namespace VKBareAPI {

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct Vertex {
        glm::vec2 pos;
        glm::vec3 color;
        glm::vec2 texCoord;

        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

            return attributeDescriptions;
        }

    };

    namespace Buffers {
        struct NEBuffers{
            VkBuffer indexBuffer;
            VkBuffer vertexBuffer;

            VkDeviceMemory indexBufferMemory;
            VkDeviceMemory vertexBufferMemory;

            VkImage textureImage;
            VkDeviceMemory textureImageMemory;
            VkImageView textureImageView;

            std::vector<VkCommandBuffer> commandBuffers;

            std::vector<VkBuffer> uniformBuffers;
            std::vector<VkDeviceMemory> uniformBuffersMemory;
            std::vector<VkDescriptorSet> descriptorSets;
        };

        const std::vector<uint16_t> indices = {
                0, 1, 2, 2, 3, 0
        };

        const std::vector<Vertex> vertices = {
                {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
                {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
                {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
                {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
        };
    }

    namespace Device {
        struct QueueFamilyIndices {
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentFamily;


            bool isComplete() {
                return graphicsFamily.has_value();
            }
        };

        const std::vector<const char *> deviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        struct NEDevice {
            VkQueue graphicsQueue;
            VkQueue presentQueue;
            VkDevice device;
            VkPhysicalDevice physicalDevice;
            QueueFamilyIndices indices;
            VkCommandPool commandPool;
            VkDescriptorPool descriptorPool;

            VKBareAPI::Buffers::NEBuffers Buffers;
        };
    }

    namespace Swapchain {
        struct SwapChainSupportDetails {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;


        };

        struct NESwapchain {
            int MAX_FRAMES_IN_FLIGHT = 2;
            int currentFrame = 0;
            bool framebufferResized = false;


            VkSwapchainKHR swapchain;
            VkFormat swapChainImageFormat;
            VkExtent2D swapChainExtent;
            SwapChainSupportDetails swapChainSupportDetails;

            std::vector<VkImage> swapChainImages;
            std::vector<VkImageView> swapChainImageViews;
            std::vector<VkFramebuffer> swapChainFramebuffers;

            std::vector<VkFence> inFlightFences;
            std::vector<VkFence> imagesInFlight;

            std::vector<VkSemaphore> imageAvailableSemaphores;
            std::vector<VkSemaphore> renderFinishedSemaphores;
        };
    }

    namespace Window {
        struct NEWindow {
            GLFWwindow* window;
            VkSurfaceKHR surface;
            std::chrono::time_point<std::chrono::system_clock> start, end;
            std::string title;
        };
    }

    namespace Pipeline {
        struct NEPipeline {
            VkPipelineLayout pipelineLayout;
            VkDescriptorSetLayout descriptorSetLayout;
            VkSampler textureSampler;
            VkPipeline pipeline;
            VkRenderPass renderPass;
        };
    }
}
#endif //NINEENGINE_SHAREDSTRUCTS_H
