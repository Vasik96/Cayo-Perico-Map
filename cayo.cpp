// Include necessary header files
#include "cayo.h"
#include "pch.h"
#include <cmath>  // Include cmath for math functions
#include <string> // Include string for std::string

// Define the target coordinates and the distance threshold
const float targetX = 4858.0f;
const float targetY = -5171.0f;
const float targetZ = 2.0f;
const float triggerDistance = 1800.0f;  // Distance at which to call the native function

// Helper function to calculate the distance between two 3D points
float GetDistance(float x1, float y1, float z1, float x2, float y2, float z2) {
    return static_cast<float>(std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2) + std::pow(z2 - z1, 2)));
}

// Define ScriptMain function
void ScriptMain() {
    while (true) {
        // Get the player's current position
        Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), true);

        // Calculate the distance from the player to the target coordinates
        float distance = GetDistance(playerPos.x, playerPos.y, playerPos.z, targetX, targetY, targetZ);

        // Call a native function based on distance
        if (distance < triggerDistance) {
            STREAMING::SET_ISLAND_ENABLED("HeistIsland", true);
            PATHFIND::_SET_AI_GLOBAL_PATH_NODES_TYPE(1);
        }
        if (distance > triggerDistance) {
            PATHFIND::_SET_AI_GLOBAL_PATH_NODES_TYPE(0);
        }




        // Call functions to set the radar as exterior and interior for the specified location
        UI::SET_RADAR_AS_EXTERIOR_THIS_FRAME();

        // Convert std::string to char* for GET_HASH_KEY
        std::string interiorName = "h4_fake_islandx";
        int interiorHash = GAMEPLAY::GET_HASH_KEY(const_cast<char*>(interiorName.c_str()));  // Convert std::string to char*

        // Set radar as interior
        UI::SET_RADAR_AS_INTERIOR_THIS_FRAME(
            interiorHash,
            static_cast<int>(4700.0),
            static_cast<int>(-5145.0),
            0,
            0
        );

        // Wait for the next frame
        WAIT(0);
    }
}

// Main function for ScriptHookV
int main() {
    // Call your script main logic
    ScriptMain();

    // Return 0 at the end of main to indicate successful execution
    return 0;
}
