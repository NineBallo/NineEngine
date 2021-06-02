//
// Created by nineball on 5/30/21.
//

#ifndef NINEENGINE_NEPIPELINE_H
#define NINEENGINE_NEPIPELINE_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>
#include "NEDevice.h"
#include "NERenderpass.h"
#include "NEWindow.h"

class NEDevice;
class NEWindow;

namespace NEVK {
    class NEPipeline {
    public:
        NEPipeline(NEDevice* device, std::shared_ptr<NERenderpass>& renderpass, std::shared_ptr<NEWindow>& window);
        VkDescriptorSetLayout descriptorSetLayout;
        operator VkPipeline() const { return pipeline; }
    private:
        void createPipeline();
      //  void createFrameBuffers();
        static std::vector<char> readFile(const std::string& filename);
        VkShaderModule createShaderModule(const std::vector<char>& code);
      //  void createDescriptorSets();
        void createTextureSampler();
        void createDescriptorSetLayout();

        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
        VkSampler textureSampler;


    private:
        NEDevice* device;
        std::shared_ptr<NERenderpass>& renderpass;
        std::shared_ptr<NEWindow>& window;
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
}



#endif //NINEENGINE_NEPIPELINE_H
