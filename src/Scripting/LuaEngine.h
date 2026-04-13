#pragma once

#include "IScript.h"

#include <memory>
#include <unordered_map>

namespace NK
{

/**
 * LuaEngine - Lua scripting implementation using Sol2
 * 
 * Dependencies: sol2 (https://github.com/ThePhD/sol2)
 * 
 * Add to CMakeLists.txt:
 *   FetchContent_Declare(
 *       sol2
 *       GIT_REPOSITORY https://github.com/ThePhD/sol2.git
 *       GIT_TAG v3.7.0
 *   )
 *   FetchContent_MakeAvailable(sol2)
 *   target_link_libraries(Neki PUBLIC sol2)
 */
class LuaEngine : public IScriptEngine
{
public:
    LuaEngine();
    ~LuaEngine() override;
    
    bool Initialize() override;
    void Shutdown() override;
    
    bool LoadScript(const std::string& name, const std::string& filepath) override;
    bool ReloadScript(const std::string& name) override;
    void Update(float deltaTime) override;
    bool Execute(const std::string& code) override;
    
    bool CallFunction(const std::string& scriptName, const std::string& functionName) override;
    bool CallFunction(const std::string& scriptName, const std::string& functionName, const std::string& arg) override;
    
private:
    bool m_initialized = false;
    
    // In real implementation:
    // std::unique_ptr<sol::state> m_lua;
    
    // Track loaded scripts for hot-reloading
    std::unordered_map<std::string, std::string> m_loadedScripts;
};

/**
 * Example Lua script:
 * 
 * -- player_controller.lua
 * 
 * function OnStart()
 *     print("Player controller started!")
 *     speed = 10.0
 * end
 * 
 * function OnUpdate(dt)
 *     local pos = entity:GetPosition()
 *     
 *     if Input.IsKeyPressed("W") then
 *         pos.z = pos.z + speed * dt
 *     end
 *     if Input.IsKeyPressed("S") then
 *         pos.z = pos.z - speed * dt
 *     end
 *     if Input.IsKeyPressed("A") then
 *         pos.x = pos.x - speed * dt
 *     end
 *     if Input.IsKeyPressed("D") then
 *         pos.x = pos.x + speed * dt
 *     end
 *     
 *     entity:SetPosition(pos)
 * end
 * 
 * function OnCollisionEnter(other)
 *     print("Collision with: " .. other:GetName())
 * end
 */

} // namespace NK