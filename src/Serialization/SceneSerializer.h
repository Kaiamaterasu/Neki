#pragma once

#include <string>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <variant>
#include <optional>
#include <glm/glm.hpp>

#include <Core-ECS/Registry.h>
#include <Components/CTransform.h>
#include <Components/CModelRenderer.h>
#include <Components/CLight.h>
#include <Components/CPhysicsBody.h>

namespace NK
{

/**
 * SceneSerializer - Serialize and deserialize game scenes
 * 
 * Supports YAML format for human-readable scene files
 * 
 * Example YAML output:
 * ```yaml
 * Scene:
 *   name: "Level1"
 *   version: "1.0"
 *   
 * entities:
 *   - id: 1
 *     name: "Player"
 *     components:
 *       Transform:
 *         position: [0, 0, 0]
 *         rotation: [0, 0, 0]
 *         scale: [1, 1, 1]
 *       ModelRenderer:
 *         modelPath: "assets/models/player.nkmodel"
 *       PhysicsBody:
 *         mass: 1.0
 *         isStatic: false
 *       
 *   - id: 2
 *     name: "Sun"
 *     components:
 *       Transform:
 *         position: [10, 20, 10]
 *       Light:
 *         type: Directional
 *         color: [1.0, 0.9, 0.8]
 *         intensity: 1.0
 * ```
 */
class SceneSerializer
{
public:
    explicit SceneSerializer(Registry& registry);
    ~SceneSerializer() = default;
    
    // =========================================================================
    // Serialization (Save)
    // =========================================================================
    
    /**
     * Serialize entire scene to YAML file
     */
    bool Serialize(const std::string& filepath);
    
    /**
     * Serialize entire scene to YAML string
     */
    std::string SerializeToString();
    
    /**
     * Serialize specific entities
     */
    bool SerializeEntities(const std::vector<Entity>& entities, const std::string& filepath);
    
    // =========================================================================
    // Deserialization (Load)
    // =========================================================================
    
    /**
     * Deserialize scene from YAML file
     */
    bool Deserialize(const std::string& filepath);
    
    /**
     * Deserialize scene from YAML string
     */
    bool DeserializeFromString(const std::string& yaml);
    
    // =========================================================================
    // Entity Operations
    // =========================================================================
    
    /**
     * Create entity from YAML node
     */
    Entity DeserializeEntity(const std::string& yaml);
    
    /**
     * Serialize single entity to YAML
     */
    std::string SerializeEntity(Entity entity);
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    void SetSceneName(const std::string& name) { m_sceneName = name; }
    std::string GetSceneName() const { return m_sceneName; }
    
    // Which component types to serialize
    void SetIncludeComponents(const std::vector<std::string>& components) { m_includeComponents = components; }
    
private:
    Registry& m_registry;
    std::string m_sceneName = "UnnamedScene";
    std::string m_version = "1.0";
    std::vector<std::string> m_includeComponents; // Empty means all
    
    // =========================================================================
    // Serialization Helpers
    // =========================================================================
    
    std::string SerializeTransform(const CTransform& transform);
    std::string SerializeModelRenderer(const CModelRenderer& model);
    std::string SerializeLight(const CLight& light);
    std::string SerializePhysicsBody(const CPhysicsBody& body);
    
    // =========================================================================
    // Deserialization Helpers
    // =========================================================================
    
    bool ParseSceneHeader(const std::string& yaml);
    std::unordered_map<std::string, std::string> ParseEntity(const std::string& entityYaml);
    
    bool DeserializeTransform(Entity entity, const std::string& yaml);
    bool DeserializeModelRenderer(Entity entity, const std::string& yaml);
    bool DeserializeLight(Entity entity, const std::string& yaml);
    bool DeserializePhysicsBody(Entity entity, const std::string& yaml);
    
    // =========================================================================
    // Utility
    // =========================================================================
    
    std::string Indent(const std::string& str, int spaces);
    std::string Vec3ToYaml(const glm::vec3& v);
    glm::vec3 YamlToVec3(const std::string& yaml);
    std::string Vec4ToYaml(const glm::vec4& v);
    glm::vec4 YamlToVec4(const std::string& yaml);
};

/**
 * Simple YAML parser for basic types
 * Note: For production, consider using a proper YAML library like yaml-cpp
 */
class SimpleYamlParser
{
public:
    static std::optional<std::string> GetString(const std::string& yaml, const std::string& key);
    static std::optional<int> GetInt(const std::string& yaml, const std::string& key);
    static std::optional<float> GetFloat(const std::string& yaml, const std::string& key);
    static std::optional<bool> GetBool(const std::string& yaml, const std::string& key);
    static std::vector<std::string> GetArray(const std::string& yaml, const std::string& key);
};

} // namespace NK