#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <functional>

#include <Core-ECS/Entity.h>

namespace NK
{

// Forward declarations
class ScriptManager;

/**
 * Base interface for all scriptable components
 * Scripts can be attached to entities and receive lifecycle callbacks
 */
class IScript
{
public:
    virtual ~IScript() = default;
    
    // Lifecycle
    virtual void OnStart() {}
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnFixedUpdate(float fixedDeltaTime) {}
    virtual void OnLateUpdate(float deltaTime) {}
    virtual void OnDestroy() {}
    virtual void OnCollisionEnter(Entity other) {}
    virtual void OnCollisionExit(Entity other) {}
    
    // Access to entity
    Entity GetEntity() const { return m_entity; }
    ScriptManager* GetScriptManager() const { return m_scriptManager; }
    
    // Template helper to get other components
    template<typename T>
    T* GetComponent()
    {
        return m_scriptManager->GetComponent<T>(m_entity);
    }
    
    template<typename T>
    bool HasComponent()
    {
        return m_scriptManager->HasComponent<T>(m_entity);
    }

protected:
    Entity m_entity;
    ScriptManager* m_scriptManager = nullptr;
    
    // Allow ScriptManager to set these
    friend class ScriptManager;
    void SetEntity(Entity e) { m_entity = e; }
    void SetScriptManager(ScriptManager* mgr) { m_scriptManager = mgr; }
};

/**
 * Script engine interface
 * Implement this to add support for different scripting languages
 */
class IScriptEngine
{
public:
    virtual ~IScriptEngine() = default;
    
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual bool LoadScript(const std::string& name, const std::string& filepath) = 0;
    virtual bool ReloadScript(const std::string& name) = 0;
    virtual void Update(float deltaTime) = 0;
    virtual bool Execute(const std::string& code) = 0;
    
    virtual bool CallFunction(const std::string& scriptName, const std::string& functionName) = 0;
    virtual bool CallFunction(const std::string& scriptName, const std::string& functionName, const std::string& arg) = 0;
};

/**
 * Script binding utilities
 * Helper functions to register C++ functions to Lua
 */
class ScriptBindings
{
public:
    using BindFunction = std::function<void()>;
    
    static void RegisterGlobals(IScriptEngine* engine);
    static void RegisterEntityAPI(IScriptEngine* engine);
    static void RegisterMathAPI(IScriptEngine* engine);
    static void RegisterInputAPI(IScriptEngine* engine);
    
private:
    static void RegisterCoreBindings();
    static void RegisterEntityBindings();
    static void RegisterMathBindings();
    static void RegisterInputBindings();
};

} // namespace NK