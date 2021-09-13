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
    renderer.createMesh("./models/sponza.obj", "model");
    renderer.loadTexture("./models/textures/lion.tga", "lion");
    renderer.loadTexture("./models/textures/background.tga", "background");
    renderer.loadTexture("./models/textures/vase_plant.tga", "vase_plant");
    renderer.loadTexture("./models/textures/sponza_arch_diff.tga", "arch");
    renderer.loadTexture("./models/textures/spnza_bricks_a_diff.tga", "bricks");
    renderer.loadTexture("./models/textures/sponza_ceiling_a_diff.tga", "ceiling");
    renderer.loadTexture("./models/textures/chain_texture.tga", "chain");
    renderer.loadTexture("./models/textures/sponza_column_a_diff.tga", "column_a");
    renderer.loadTexture("./models/textures/sponza_column_b_diff.tga", "column_b");
    renderer.loadTexture("./models/textures/sponza_column_c_diff.tga", "column_c");
    renderer.loadTexture("./models/textures/sponza_details_diff.tga", "details");
    renderer.loadTexture("./models/textures/sponza_fabric_diff.tga", "fabric");
    renderer.loadTexture("./models/textures/sponza_curtain_diff.tga", "curtain");
    renderer.loadTexture("./models/textures/sponza_fabric_blue_diff.tga", "fabric_blue");
    renderer.loadTexture("./models/textures/sponza_fabric_green_diff.tga", "fabric_green");
    renderer.loadTexture("./models/textures/sponza_curtain_green_diff.tga", "curtain_green");
    renderer.loadTexture("./models/textures/sponza_curtain_blue_diff.tga", "curtain_blue");
    renderer.loadTexture("./models/textures/sponza_flagpole_diff.tga", "flagpole");
    renderer.loadTexture("./models/textures/sponza_floor_a_diff.tga", "floor_a");
    renderer.loadTexture("./models/textures/sponza_thorn_diff.tga", "thorn");
    renderer.loadTexture("./models/textures/sponza_roof_diff.tga", "roof");
    renderer.loadTexture("./models/textures/vase_dif.tga", "vase");
    renderer.loadTexture("./models/textures/vase_hanging.tga", "vase_hanging");
    renderer.loadTexture("./models/textures/vase_round.tga", "vase_round");

    renderer.loadTexture("./models/textures/vase_round.tga", "extra");

    Entity mainEntity = ecs.createEntity(0);
    Position position;
    position.scalar.x = 0.1;
    position.scalar.y = 0.1;
    position.scalar.z = 0.1;
    ecs.addComponent(mainEntity, position);

    std::string textures[25] = {"lion", "background", "vase_plant", "arch", "bricks", "ceiling", "floor_a", "column_a",
                                "column_b", "column_c", "details", "fabric", "flagpole", "roof", "fabric_green",
                                "curtain_green", "curtain_blue", "curtain", "chain", "thorn", "fabric_blue", "vase", "vase_hanging",
                                "vase_round", "extra"};

    std::string index[25] = {"Material__25", "Material__298", "Material__57", "arch", "bricks", "ceiling", "floor", "column_a",
                             "column_b", "column_c", "details", "fabric_a", "flagpole", "roof", "fabric_e",
                             "fabric_f", "fabric_g", "fabric_c", "chain", "leaf", "fabric_d", "vase", "vase_hanging",
                             "vase_round", "Material__47"
    };

    renderer.makeRenderable(mainEntity, NE_SHADER_TEXTURE_BIT, "model", textures, index);


    Keyboard keyboard(renderer.getWindow(0));
    Mouse mouse(renderer.getWindow(0));

    auto& cameraRef = ecs.getComponent<Camera>(player);

    keyboard.registerMovement(GLFW_KEY_W, {0, 0, 0}, 20, std::addressof(cameraRef.Pos), std::addressof(cameraRef.Angle));
    keyboard.registerMovement(GLFW_KEY_S, {0,0,0}, -20, std::addressof(cameraRef.Pos), std::addressof(cameraRef.Angle));
    keyboard.registerMovement(GLFW_KEY_A, {0,90,0}, -20, std::addressof(cameraRef.Pos), std::addressof(cameraRef.Angle));
    keyboard.registerMovement(GLFW_KEY_D, {0,90,0}, 20, std::addressof(cameraRef.Pos), std::addressof(cameraRef.Angle));
    keyboard.registerValue(GLFW_KEY_LEFT_CONTROL, -20, 1, std::addressof(cameraRef.Pos));
    keyboard.registerValue(GLFW_KEY_SPACE, 20, 1, std::addressof(cameraRef.Pos));

    mouse.registerMovement(0.1, std::addressof(cameraRef.Angle));

    while(!renderer.shouldExit()) {
        glfwPollEvents();

        mouse.tick();
        keyboard.tick();
        renderer.tick();
    }
}

