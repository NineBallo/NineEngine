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
    renderer.createMesh("./models/bananaconcert.obj", "bananaConcert");
    renderer.createMesh("./models/monkey.obj", "monkey");

    renderer.createMaterial("./shaders/mesh.vert.spv", "./shaders/basic.frag.spv", "basicMesh");

    Position position;
    ecs.addComponent(banana, position);

    renderer.makeRenderable(banana, "basicMesh", "monkey");


    for(uint32_t x = 1; x <= 9; x++) {
        for(uint32_t y = 1; y <= 9; y++) {
            Entity entity;
            entity = ecs.createEntity(0);

              std::cout << entity << std::endl;

             // std::cout << x << "   " << y << std::endl;

              Position coords;
              coords.coordinates.x = (x*3);
              coords.coordinates.y = (y*3)+50;

              ecs.addComponent(entity, coords);

            renderer.makeRenderable(entity, "basicMesh", "monkey");
        }
    }



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


    std::chrono::time_point<std::chrono::steady_clock> currentEntity;
    std::chrono::time_point<std::chrono::steady_clock> lastEntity;

    uint32_t monkeyCounter;

    lastEntity = std::chrono::steady_clock::now();


    while(!renderer.shouldExit()) {
        //currentEntity = std::chrono::steady_clock::now();
        //std::chrono::duration<float> duration = std::chrono::duration_cast<std::chrono::duration<float>>(currentEntity - lastEntity);
        //float seconds = (duration.count());
        //if(seconds > 3) {
        //    Entity entity;
        //    entity = ecs.createEntity(0);
//
        //    std::cout << entity << std::endl;
//
        //    Position coords;
        //    coords.coordinates.x = (monkeyCounter*3) + 10;
//
        //    ecs.addComponent(entity, coords);
//
        //    renderer.makeRenderable(entity, "basicMesh", "monkey");
//
        //    monkeyCounter++;
        //    lastEntity = currentEntity;
        //}

        mouse.tick();
        keyboard.tick();
        renderer.tick();
    }

}