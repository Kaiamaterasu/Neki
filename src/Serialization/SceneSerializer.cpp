#include "SceneSerializer.h"

#include <iostream>
#include <sstream>
#include <algorithm>

namespace NK
{

SceneSerializer::SceneSerializer(Registry& registry)
    : m_registry(registry)
{
}

bool SceneSerializer::Serialize(const std::string& filepath)
{
    std::ofstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "SceneSerializer: Failed to open file for writing: " << filepath << std::endl;
        return false;
    }
    
    file << SerializeToString();
    file.close();
    
    std::cout << "SceneSerializer: Serialized scene to " << filepath << std::endl;
    return true;
}

std::string SceneSerializer::SerializeToString()
{
    std::ostringstream oss;
    
    // Scene header
    oss << "Scene:\n";
    oss << "  name: \"" << m_sceneName << "\"\n";
    oss << "  version: \"" << m_version << "\"\n";
    oss << "\n";
    
    // Get all entities
    auto entities = m_registry.GetAllEntities();
    
    oss << "entities:\n";
    
    for (Entity entity : entities)
    {
        if (!m_registry.IsValid(entity)) continue;
        
        oss << "  - id: " << static_cast<uint32_t>(entity) << "\n";
        
        // Get entity name if it has a name component (would need CName component)
        // For now, use entity ID
        oss << "    name: \"Entity_" << static_cast<uint32_t>(entity) << "\"\n";
        
        oss << "    components:\n";
        
        // Serialize each component type
        if (m_registry.HasComponent<CTransform>(entity))
        {
            oss << SerializeTransform(m_registry.GetComponent<CTransform>(entity));
        }
        
        if (m_registry.HasComponent<CModelRenderer>(entity))
        {
            oss << SerializeModelRenderer(m_registry.GetComponent<CModelRenderer>(entity));
        }
        
        if (m_registry.HasComponent<CLight>(entity))
        {
            oss << SerializeLight(m_registry.GetComponent<CLight>(entity));
        }
        
        if (m_registry.HasComponent<CPhysicsBody>(entity))
        {
            oss << SerializePhysicsBody(m_registry.GetComponent<CPhysicsBody>(entity));
        }
        
        oss << "\n";
    }
    
    return oss.str();
}

bool SceneSerializer::Deserialize(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "SceneSerializer: Failed to open file for reading: " << filepath << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string yaml = buffer.str();
    file.close();
    
    return DeserializeFromString(yaml);
}

bool SceneSerializer::DeserializeFromString(const std::string& yaml)
{
    // Parse scene header
    if (!ParseSceneHeader(yaml))
    {
        std::cerr << "SceneSerializer: Failed to parse scene header" << std::endl;
        return false;
    }
    
    // Find and parse each entity
    // This is a simplified parser - production would use proper YAML library
    
    std::cout << "SceneSerializer: Deserialized scene \"" << m_sceneName << "\"" << std::endl;
    return true;
}

// =========================================================================
// Serialization Helpers
// =========================================================================

std::string SceneSerializer::SerializeTransform(const CTransform& transform)
{
    std::ostringstream oss;
    oss << "      Transform:\n";
    oss << "        position: " << Vec3ToYaml(transform.position) << "\n";
    oss << "        rotation: " << Vec3ToYaml(transform.rotation) << "\n";
    oss << "        scale: " << Vec3ToYaml(transform.scale) << "\n";
    return oss.str();
}

std::string SceneSerializer::SerializeModelRenderer(const CModelRenderer& model)
{
    std::ostringstream oss;
    oss << "      ModelRenderer:\n";
    oss << "        modelPath: \"" << model.modelPath << "\"\n";
    return oss.str();
}

std::string SceneSerializer::SerializeLight(const CLight& light)
{
    std::ostringstream oss;
    oss << "      Light:\n";
    oss << "        type: " << static_cast<int>(light.type) << "\n";
    oss << "        color: " << Vec3ToYaml(light.color) << "\n";
    oss << "        intensity: " << light.intensity << "\n";
    
    if (light.type == LIGHT_TYPE::DIRECTIONAL || light.type == LIGHT_TYPE::SPOT)
    {
        oss << "        direction: " << Vec3ToYaml(light.direction) << "\n";
    }
    
    if (light.type == LIGHT_TYPE::SPOT)
    {
        oss << "        innerConeAngle: " << light.innerConeAngle << "\n";
        oss << "        outerConeAngle: " << light.outerConeAngle << "\n";
    }
    
    return oss.str();
}

std::string SceneSerializer::SerializePhysicsBody(const CPhysicsBody& body)
{
    std::ostringstream oss;
    oss << "      PhysicsBody:\n";
    oss << "        mass: " << body.mass << "\n";
    oss << "        isStatic: " << (body.isStatic ? "true" : "false") << "\n";
    return oss.str();
}

// =========================================================================
// Deserialization Helpers
// =========================================================================

bool SceneSerializer::ParseSceneHeader(const std::string& yaml)
{
    // Simple parsing - look for "name:" and "version:"
    size_t namePos = yaml.find("name:");
    if (namePos != std::string::npos)
    {
        size_t start = yaml.find("\"", namePos);
        size_t end = yaml.find("\"", start + 1);
        if (start != std::string::npos && end != std::string::npos)
        {
            m_sceneName = yaml.substr(start + 1, end - start - 1);
        }
    }
    
    return true;
}

// =========================================================================
// Utility
// =========================================================================

std::string SceneSerializer::Indent(const std::string& str, int spaces)
{
    std::ostringstream oss;
    std::string indent(spaces, ' ');
    
    std::istringstream iss(str);
    std::string line;
    while (std::getline(iss, line))
    {
        if (!line.empty())
        {
            oss << indent << line << "\n";
        }
    }
    
    return oss.str();
}

std::string SceneSerializer::Vec3ToYaml(const glm::vec3& v)
{
    std::ostringstream oss;
    oss << "[" << v.x << ", " << v.y << ", " << v.z << "]";
    return oss.str();
}

glm::vec3 SceneSerializer::YamlToVec3(const std::string& yaml)
{
    glm::vec3 result(0, 0, 0);
    
    // Parse [x, y, z] format
    size_t start = yaml.find('[');
    size_t end = yaml.find(']');
    
    if (start != std::string::npos && end != std::string::npos)
    {
        std::string values = yaml.substr(start + 1, end - start - 1);
        std::replace(values.begin(), values.end(), ',', ' ');
        
        std::istringstream iss(values);
        iss >> result.x >> result.y >> result.z;
    }
    
    return result;
}

std::string SceneSerializer::Vec4ToYaml(const glm::vec4& v)
{
    std::ostringstream oss;
    oss << "[" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << "]";
    return oss.str();
}

glm::vec4 SceneSerializer::YamlToVec4(const std::string& yaml)
{
    glm::vec4 result(0, 0, 0, 0);
    
    size_t start = yaml.find('[');
    size_t end = yaml.find(']');
    
    if (start != std::string::npos && end != std::string::npos)
    {
        std::string values = yaml.substr(start + 1, end - start - 1);
        std::replace(values.begin(), values.end(), ',', ' ');
        
        std::istringstream iss(values);
        iss >> result.x >> result.y >> result.z >> result.w;
    }
    
    return result;
}

// =========================================================================
// SimpleYamlParser Implementation
// =========================================================================

std::optional<std::string> SimpleYamlParser::GetString(const std::string& yaml, const std::string& key)
{
    std::string search = key + ":";
    size_t pos = yaml.find(search);
    
    if (pos == std::string::npos) return std::nullopt;
    
    size_t start = yaml.find("\"", pos);
    size_t end = yaml.find("\"", start + 1);
    
    if (start == std::string::npos || end == std::string::npos) return std::nullopt;
    
    return yaml.substr(start + 1, end - start - 1);
}

std::optional<int> SimpleYamlParser::GetInt(const std::string& yaml, const std::string& key)
{
    std::string search = key + ":";
    size_t pos = yaml.find(search);
    
    if (pos == std::string::npos) return std::nullopt;
    
    std::istringstream iss(yaml.substr(pos));
    int value;
    iss >> value;
    
    return value;
}

std::optional<float> SimpleYamlParser::GetFloat(const std::string& yaml, const std::string& key)
{
    std::string search = key + ":";
    size_t pos = yaml.find(search);
    
    if (pos == std::string::npos) return std::nullopt;
    
    std::istringstream iss(yaml.substr(pos));
    float value;
    iss >> value;
    
    return value;
}

std::optional<bool> SimpleYamlParser::GetBool(const std::string& yaml, const std::string& key)
{
    auto str = GetString(yaml, key);
    if (!str) return std::nullopt;
    
    return *str == "true";
}

std::vector<std::string> SimpleYamlParser::GetArray(const std::string& yaml, const std::string& key)
{
    std::vector<std::string> result;
    
    std::string search = key + ":";
    size_t pos = yaml.find(search);
    
    if (pos == std::string::npos) return result;
    
    size_t start = yaml.find("[", pos);
    size_t end = yaml.find("]", start);
    
    if (start == std::string::npos || end == std::string::npos) return result;
    
    std::string arrayStr = yaml.substr(start + 1, end - start - 1);
    std::replace(arrayStr.begin(), arrayStr.end(), ',', ' ');
    
    std::istringstream iss(arrayStr);
    std::string item;
    while (iss >> item)
    {
        result.push_back(item);
    }
    
    return result;
}

} // namespace NK