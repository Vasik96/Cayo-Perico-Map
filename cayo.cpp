// Include necessary header files
#include "cayo.h"
#include "pch.h"
#include <cmath>  // Include cmath for math functions
#include <string> // Include string for std::string
#include <ctime>  // Include ctime for random seed
#include <iostream> // Include iostream for debug output
#include <thread>
#include <chrono>




// Define the target coordinates and the distance threshold
const float targetX = 4858.0f;
const float targetY = -5171.0f;
const float targetZ = 2.0f;
const float triggerDistance = 1800.0f;  // Distance at which to call the native function
const int maxAttempts = 10;  // Number of attempts to find a safe location per radius
const float minSafeHeight = 1.5f;  // Minimum height above water to be considered safe
const float minRespawnDistance = 80.0f;  // Minimum distance from the original location
const float initialSearchRadius = 170.0f;  // Initial radius for searching respawn location
const float searchRadiusDecrement = 50.0f;  // Decrement for search radius
const float minSearchRadius = 40.0f;  // Minimum search radius before resetting

bool cayoEnabled = false;

// Helper function to calculate the distance between two 3D points
float GetDistance(float x1, float y1, float z1, float x2, float y2, float z2) {
    return static_cast<float>(std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2) + std::pow(z2 - z1, 2)));
}

// Function to get a random float between min and max
float GetRandomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

// Function to check if a coordinate is in water
bool IsInWater(float x, float y, float z) {
    float waterHeight;
    return WATER::GET_WATER_HEIGHT_NO_WAVES(x, y, z, &waterHeight) && z < waterHeight;
}


void WaitMultipleTimes(int durationMs, int times) {
    for (int i = 0; i < times; ++i) {
        WAIT(durationMs);
    }
}

struct Vector4 {
    float x;
    float y;
    float z;
};

// Define the fallback respawn location (e.g., Cayo Perico airstrip coordinates)
Vector4 fallbackLocation = { 4475, -4490, 4 };

void HandlePlayerRespawn(Vector3 playerPos) {
    // Define search parameters
    float respawnX, respawnY, respawnZ;
    bool foundSafeLocation = false;
    float searchRadius = initialSearchRadius;
    int attempts = 0;

    while (!foundSafeLocation && attempts < maxAttempts) {
        // Attempt to find a safe respawn location within the search radius
        for (int i = 0; i < maxAttempts; i++) {
            respawnX = playerPos.x + GetRandomFloat(-searchRadius, searchRadius);
            respawnY = playerPos.y + GetRandomFloat(-searchRadius, searchRadius);
            float groundZ, waterHeight;

            // Get ground Z coordinate
            if (GAMEPLAY::GET_GROUND_Z_FOR_3D_COORD(respawnX, respawnY, playerPos.z + 100.0f, &groundZ, false)) {
                respawnZ = groundZ;

                // Check if the location is safe
                if (!IsInWater(respawnX, respawnY, respawnZ) &&
                    (respawnZ - WATER::GET_WATER_HEIGHT_NO_WAVES(respawnX, respawnY, respawnZ, &waterHeight) > minSafeHeight) &&
                    GetDistance(playerPos.x, playerPos.y, playerPos.z, respawnX, respawnY, respawnZ) > minRespawnDistance) {

                    foundSafeLocation = true;
                    break;
                }
            }
        }

        if (!foundSafeLocation) {
            searchRadius -= searchRadiusDecrement;
            attempts++;
            if (searchRadius < minSearchRadius) {
                searchRadius = initialSearchRadius;
            }
        }
    }

    // Fallback to a predefined location if no safe location is found
    if (!foundSafeLocation) {
        respawnX = fallbackLocation.x;
        respawnY = fallbackLocation.y;
        respawnZ = fallbackLocation.z;
    }

    // Wait before executing the respawn process
    //slow down the code, so the respawn looks like its vanilla
    WaitMultipleTimes(1000, 300);


    

    // Get the player's heading
    float heading = ENTITY::GET_ENTITY_HEADING(PLAYER::PLAYER_PED_ID());

    // Network resurrect the player
    NETWORK::NETWORK_RESURRECT_LOCAL_PLAYER(
        respawnX,        // float x
        respawnY,        // float y
        respawnZ,        // float z
        heading,         // float heading
        0,               // int nInvincibilityTime
        TRUE             // BOOL bLeaveDeadPed
    );

    // Respawn player at the determined coordinates
    ENTITY::SET_ENTITY_COORDS(
        PLAYER::PLAYER_PED_ID(),
        respawnX,
        respawnY,
        respawnZ,
        true,
        false,
        false,
        true
    );

    // Disable any assigned tasks
    AI::CLEAR_PED_TASKS_IMMEDIATELY(PLAYER::PLAYER_PED_ID());
    // Clear player's wanted level
    PLAYER::CLEAR_PLAYER_WANTED_LEVEL(PLAYER::PLAYER_ID());
}






// Define ScriptMain function
void ScriptMain() {
    srand(static_cast<unsigned int>(time(0)));  // Initialize random seed

    while (true) {
        // Get the player's current position
        Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), true);

        // Calculate the distance from the player to the target coordinates
        float distance = GetDistance(playerPos.x, playerPos.y, playerPos.z, targetX, targetY, targetZ);

        // Call a native function based on distance
        if (distance < triggerDistance) {
            STREAMING::SET_ISLAND_ENABLED("HeistIsland", true);
            PATHFIND::_SET_AI_GLOBAL_PATH_NODES_TYPE(1);
            AUDIO::SET_AMBIENT_ZONE_LIST_STATE_PERSISTENT(const_cast<char*>("AZL_DLC_Hei4_Island_Disabled_Zones"), false, true);
            AUDIO::SET_AMBIENT_ZONE_LIST_STATE_PERSISTENT(const_cast<char*>("AZL_DLC_Hei4_Island_Disabled_Zones"), true, true);
            AUDIO::SET_AMBIENT_ZONE_LIST_STATE_PERSISTENT(const_cast<char*>("AZL_DLC_Hei4_Island_Disabled_Zones"), true, true);
            AI::SET_SCENARIO_GROUP_ENABLED(const_cast<char*>("Heist_Island_Peds"), true);
            HUD::SET_USE_ISLAND_MAP(true);
            int zoneId = ZONE::GET_ZONE_FROM_NAME_ID(const_cast<char*>("PrLog"));
            ZONE::SET_ZONE_ENABLED(zoneId, false);
            cayoEnabled = true;
        }
        else if (distance > triggerDistance && cayoEnabled) {
            PATHFIND::_SET_AI_GLOBAL_PATH_NODES_TYPE(0);
            AUDIO::SET_AMBIENT_ZONE_LIST_STATE_PERSISTENT(const_cast<char*>("AZL_DLC_Hei4_Island_Disabled_Zones"), true, true);
            AUDIO::SET_AMBIENT_ZONE_LIST_STATE_PERSISTENT(const_cast<char*>("AZL_DLC_Hei4_Island_Disabled_Zones"), false, true);
            AUDIO::SET_AMBIENT_ZONE_LIST_STATE_PERSISTENT(const_cast<char*>("AZL_DLC_Hei4_Island_Disabled_Zones"), false, true);
            AI::SET_SCENARIO_GROUP_ENABLED(const_cast<char*>("Heist_Island_Peds"), false);
            HUD::SET_USE_ISLAND_MAP(false);
            int zoneId = ZONE::GET_ZONE_FROM_NAME_ID(const_cast<char*>("PrLog"));
            ZONE::SET_ZONE_ENABLED(zoneId, true);
            cayoEnabled = false;
        }

        // Monitor player's health and respawn at a safe location around death position if the player dies and cayoEnabled is true
        if (cayoEnabled && ENTITY::IS_ENTITY_DEAD(PLAYER::PLAYER_PED_ID())) {
            HandlePlayerRespawn(playerPos);
        }

        // Call functions to set the radar as exterior and interior for the specified location
        UI::SET_RADAR_AS_EXTERIOR_THIS_FRAME();

        // Convert std::string to char* for GET_HASH_KEY
        std::string interiorName = "h4_fake_islandx";
        int interiorHash = GAMEPLAY::GET_HASH_KEY(const_cast<char*>(interiorName.c_str()));  // Convert std::string to char*

        // Set radar as interior
        UI::SET_RADAR_AS_INTERIOR_THIS_FRAME(
            interiorHash,
            static_cast<int>(4700.0f),
            static_cast<int>(-5145.0f),
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
