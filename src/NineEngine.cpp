//
// Created by nineball on 5/22/21.
//

#include "NineEngine.h"
#include "Modules/Graphics/Vulkan/include/Vulkan.h"
#include "ECS.h"
#include "iostream"
#include "Modules/IO/Keyboard.h"
#include "Modules/IO/Mouse.h"
#include <glm/ext.hpp>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

float getTimeToComplete(std::function<void()> &&function) {
    std::chrono::time_point<std::chrono::steady_clock> start, end;

    start = std::chrono::steady_clock::now();
    function();
    end = std::chrono::steady_clock::now();

    std::chrono::duration<float> duration = std::chrono::duration_cast<std::chrono::duration<float>>(end - start);
    return duration.count();
}

int main(){
    ECS &ecs = ECS::Get();

    Entity player = ecs.createEntity(0);
    std::cout << "PlayerID: " << player << std::endl;

    Camera camera {};

    ecs.registerComponent<Camera>();
    ecs.addComponent(player, camera);

    Vulkan renderer(ecs, player);


    renderer.createMaterial(NE_SHADER_TEXTURE_BIT);
    renderer.createMesh("./models/viking_room.obj", "room");
    renderer.loadTexture("./models/textures/viking_room.png", "roomtex");

    Entity mainEntity = ecs.createEntity(0);
    Position position;
    position.rotations.x = 270.f;
    ecs.addComponent(mainEntity, position);

    renderer.makeRenderable(mainEntity, NE_SHADER_TEXTURE_BIT, "room", "roomtex");
  //  for(uint32_t x = 1; x <= 50; x++) {
  //      for(uint32_t y = 1; y <= 50; y++) {
  //          Entity entity;
  //          entity = ecs.createEntity(0);
  //            std::cout << entity << std::endl;
//
  //            Position coords;
  //            coords.coordinates.x = (x*0.9);
  //            coords.coordinates.y = (y*0.9);
//
  //            ecs.addComponent(entity, coords);
//
  //            renderer.makeRenderable(entity, "basicMesh", "monkey");
  //      }
  //  }

    Keyboard keyboard(renderer.getWindow(0));
    Mouse mouse(renderer.getWindow(0));

    auto& cameraRef = ecs.getComponent<Camera>(player);

    keyboard.registerMovement(GLFW_KEY_W, {0, 0, 0}, 5, std::addressof(cameraRef.Pos), std::addressof(cameraRef.Angle));
    keyboard.registerMovement(GLFW_KEY_S, {0,0,0}, -5, std::addressof(cameraRef.Pos), std::addressof(cameraRef.Angle));
    keyboard.registerMovement(GLFW_KEY_A, {0,90,0}, -5, std::addressof(cameraRef.Pos), std::addressof(cameraRef.Angle));
    keyboard.registerMovement(GLFW_KEY_D, {0,90,0}, 5, std::addressof(cameraRef.Pos), std::addressof(cameraRef.Angle));
    keyboard.registerValue(GLFW_KEY_LEFT_SHIFT, -5, 1, std::addressof(cameraRef.Pos));
    keyboard.registerValue(GLFW_KEY_SPACE, 5, 1, std::addressof(cameraRef.Pos));

    mouse.registerMovement(0.1, std::addressof(cameraRef.Angle));

    std::chrono::time_point<std::chrono::steady_clock> currentTick, lastTick;

    float lastRenderTime;

    while(!renderer.shouldExit()) {
        glfwPollEvents();

        ///Calculate FPS
        currentTick = std::chrono::steady_clock::now();
        std::chrono::duration<float> duration = std::chrono::duration_cast<std::chrono::duration<float>>(currentTick - lastTick);
        float seconds = duration.count();
        std::string FPS = std::to_string((uint32_t)(std::floor(1/seconds)));
        lastTick = currentTick;

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("FPS");
        ImGui::Text("%s", FPS.c_str());
        ImGui::End();

        ImGui::Begin("Timings");
        std::string mouseTime = "Mouse polling time: " + std::to_string(getTimeToComplete([&]{mouse.tick();}) * 1000) + "ms";
        ImGui::Text("%s", mouseTime.c_str());

        std::string keyboardTime = "Keyboard polling time: " + std::to_string(getTimeToComplete([&]{keyboard.tick();}) * 1000) + "ms";
        ImGui::Text("%s", keyboardTime.c_str());

        std::string renderString = "Render Time: " + std::to_string(lastRenderTime * 1000) + "ms";
        ImGui::Text("%s", renderString.c_str());
        ImGui::End();

        lastRenderTime = getTimeToComplete([&]{renderer.tick();});
    }
}

