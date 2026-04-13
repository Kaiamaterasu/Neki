#include "ScriptManager.h"

#include <iostream>
#include <algorithm>

namespace NK
{

ScriptManager::ScriptManager(Registry& registry)
    : m_registry(registry)
{
}

ScriptManager::~ScriptManager()
{
    Shutdown();
}

bool ScriptManager::Initialize()
{
    std::cout << "ScriptManager: Initializing..." << std::endl;
    
    // Create Lua engine
    m_scriptEngine = std::make_unique<LuaEngine>();
    
    if (!m_scriptEngine->Initialize())
    {
        std::cerr << "ScriptManager: Failed to initialize script engine!" << std::endl;
        return false;
    }
    
    // Register API bindings
    ScriptBindings::RegisterGlobals(m_scriptEngine.get());
    ScriptBindings::RegisterEntityAPI(m_scriptEngine.get());
    ScriptBindings::RegisterMathAPI(m_scriptEngine.get());
    ScriptBindings::RegisterInputAPI(m_scriptEngine.get());
    
    m_initialized = true;
    std::cout << "ScriptManager: Initialized successfully" << std::endl;
    return true;
}

void ScriptManager::Shutdown()
{
    if (!m_initialized) return;
    
    std::cout << "ScriptManager: Shutting down..." << std::endl;
    
    // Destroy all scripts
    for (auto& [entity, scripts] : m_entityScripts)
    {
        for (auto& script : scripts)
        {
            if (script)
            {
                script->OnDestroy();
            }
        }
    }
    
    m_entityScripts.clear();
    
    if (m_scriptEngine)
    {
        m_scriptEngine->Shutdown();
        m_scriptEngine.reset();
    }
    
    m_initialized = false;
    std::cout << "ScriptManager: Shutdown complete" << std::endl;
}

void ScriptManager::Update(float deltaTime)
{
    if (!m_initialized) return;
    
    // Update all scripts
    for (auto& [entity, scripts] : m_entityScripts)
    {
        // Skip disabled entities
        if (m_registry.HasComponent<CDisabled>(entity)) continue;
        
        for (auto& script : scripts)
        {
            if (script)
            {
                script->OnUpdate(deltaTime);
            }
        }
    }
}

void ScriptManager::FixedUpdate(float fixedDeltaTime)
{
    if (!m_initialized) return;
    
    for (auto& [entity, scripts] : m_entityScripts)
    {
        if (m_registry.HasComponent<CDisabled>(entity)) continue;
        
        for (auto& script : scripts)
        {
            if (script)
            {
                script->OnFixedUpdate(fixedDeltaTime);
            }
        }
    }
}

void ScriptManager::LateUpdate(float deltaTime)
{
    if (!m_initialized) return;
    
    for (auto& [entity, scripts] : m_entityScripts)
    {
        if (m_registry.HasComponent<CDisabled>(entity)) continue;
        
        for (auto& script : scripts)
        {
            if (script)
            {
                script->OnLateUpdate(deltaTime);
            }
        }
    }
}

void ScriptManager::OnCollisionEnter(Entity entity, Entity other)
{
    auto it = m_entityScripts.find(entity);
    if (it != m_entityScripts.end())
    {
        for (auto& script : it->second)
        {
            if (script)
            {
                script->OnCollisionEnter(other);
            }
        }
    }
}

void ScriptManager::OnCollisionExit(Entity entity, Entity other)
{
    auto it = m_entityScripts.find(entity);
    if (it != m_entityScripts.end())
    {
        for (auto& script : it->second)
        {
            if (script)
            {
                script->OnCollisionExit(other);
            }
        }
    }
}

bool ScriptManager::LoadScript(const std::string& name, const std::string& filepath)
{
    if (!m_scriptEngine) return false;
    return m_scriptEngine->LoadScript(name, filepath);
}

bool ScriptManager::ReloadScript(const std::string& name)
{
    if (!m_scriptEngine) return false;
    return m_scriptEngine->ReloadScript(name);
}

void ScriptManager::ReloadAllScripts()
{
    for (const auto& [name, filepath] : m_loadedScripts)
    {
        ReloadScript(name);
    }
}

// Component access helpers
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

// Explicit template instantiations for common components
template CTransform* ScriptManager::GetComponent<CTransform>(Entity);
template CPhysicsBody* ScriptManager::GetComponent<CPhysicsBody>(Entity);
template CModelRenderer* ScriptManager::GetComponent<CModelRenderer>(Entity);
template CInput* ScriptManager::GetComponent<CInput>(Entity);

template bool ScriptManager::HasComponent<CTransform>(Entity);
template bool ScriptManager::HasComponent<CPhysicsBody>(Entity);
template bool ScriptManager::HasComponent<CModelRenderer>(Entity);
template bool ScriptManager::HasComponent<CInput>(Entity);

} // namespace NK