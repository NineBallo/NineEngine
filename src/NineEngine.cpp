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




int main(){
    ECS &ecs = ECS::Get();

    Entity player = ecs.createEntity(0);
    std::cout << "PlayerID: " << player << std::endl;

    Camera camera {};

    ecs.registerComponent<Camera>();
    ecs.addComponent(player, camera);

    Vulkan renderer(ecs, player);

    Entity banana = ecs.createEntity(0);
    renderer.loadMesh("./models/bananaconcert.obj", banana);

    Keyboard keyboard(renderer.getWindow(0));
    Mouse mouse(renderer.getWindow(0));

    auto& cameraRef = ecs.getComponent<Camera>(player);

    keyboard.registerMovement(GLFW_KEY_W, {0, 0, 0}, 10, std::addressof(cameraRef.Pos), std::addressof(cameraRef.Angle));
    keyboard.registerMovement(GLFW_KEY_S, {0,0,0}, -10, std::addressof(cameraRef.Pos), std::addressof(cameraRef.Angle));
    keyboard.registerMovement(GLFW_KEY_A, {0,90,0}, -10, std::addressof(cameraRef.Pos), std::addressof(cameraRef.Angle));
    keyboard.registerMovement(GLFW_KEY_D, {0,90,0}, 10, std::addressof(cameraRef.Pos), std::addressof(cameraRef.Angle));
    keyboard.registerValue(GLFW_KEY_LEFT_SHIFT, -10.0, 1, std::addressof(cameraRef.Pos));
    keyboard.registerValue(GLFW_KEY_SPACE, 10.0, 1, std::addressof(cameraRef.Pos));

    mouse.registerMovement(15, std::addressof(cameraRef.Angle));


    while(!renderer.shouldExit()) {
        mouse.tick();
        keyboard.tick();
        renderer.tick();
    }

}