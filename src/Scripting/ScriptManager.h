#pragma once

#include "IScript.h"

#include <Core-ECS/Registry.h>
#include <Components/CTransform.h>
#include <Components/CPhysicsBody.h>
#include <Components/CModelRenderer.h>
#include <Components/CInput.h>
#include <Components/CDisabled.h>

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

namespace NK
{

/**
 * ScriptManager - Manages all scripts attached to entities
 * 
 * Usage:
 *   ScriptManager scriptManager(registry);
 *   scriptManager.Initialize();
 *   
 *   // Create entity with script
 *   Entity player = registry.CreateEntity();
 *   scriptManager.AddScript<PlayerController>(player, "player_controller.lua");
 *   
 *   // In game loop
 *   scriptManager.Update(deltaTime);
 */
class ScriptManager
{
public:
    explicit ScriptManager(Registry& registry);
    ~ScriptManager();
    
    // Lifecycle
    bool Initialize();
    void Shutdown();
    
    // Update loops
    void Update(float deltaTime);
    void FixedUpdate(float fixedDeltaTime);
    void LateUpdate(float deltaTime);
    
    // Collision events
    void OnCollisionEnter(Entity entity, Entity other);
    void OnCollisionExit(Entity entity, Entity other);
    
    // Script management
    /**
     * Add a C++ script to an entity
     * Usage: AddScript<PlayerController>(entity)
     */
    template<typename T>
    T* AddScript(Entity entity)
    {
        static_assert(std::is_base_of<IScript, T>::value, "T must derive from IScript");
        
        auto script = std::make_unique<T>();
        script->SetEntity(entity);
        script->SetScriptManager(this);
        
        T* scriptPtr = script.get();
        
        m_entityScripts[entity].push_back(std::move(script));
        
        // Call OnStart for the newly added script
        scriptPtr->OnStart();
        
        return scriptPtr;
    }
    
    /**
     * Add a Lua script to an entity
     * Usage: AddLuaScript(entity, "player_controller", "scripts/player.lua")
     */
    void AddLuaScript(Entity entity, const std::string& scriptName, const std::string& filepath);
    
    /**
     * Remove all scripts from an entity
     */
    void RemoveScripts(Entity entity);
    
    /**
     * Get all scripts attached to an entity
     */
    const std::vector<std::unique_ptr<IScript>>* GetScripts(Entity entity) const;
    
    // Script loading
    bool LoadScript(const std::string& name, const std::string& filepath);
    bool ReloadScript(const std::string& name);
    void ReloadAllScripts();
    
    // Component access (for scripts)
    template<typename T>
    T* GetComponent(Entity entity);
    
    template<typename T>
    bool HasComponent(Entity entity);
    
    // Utility
    bool IsInitialized() const { return m_initialized; }
    
private:
    Registry& m_registry;
    std::unique_ptr<IScriptEngine> m_scriptEngine;
    
    // Entity to scripts mapping
    std::unordered_map<Entity, std::vector<std::unique_ptr<IScript>>> m_entityScripts;
    
    // Loaded script files for hot-reloading
    std::unordered_map<std::string, std::string> m_loadedScripts;
    
    bool m_initialized = false;
};

// Inline template implementations for header-only convenience
template<typename T>
T* ScriptManager::GetComponent(Entity entity)
{
    if (m_registry.HasComponent<T>(entity))
    {
        return &m_registry.GetComponent<T>(entity);
    }
    return nullptr;
}

template<typename T>
bool ScriptManager::HasComponent(Entity entity)
{
    return m_registry.HasComponent<T>(entity);
}

} // namespace NK