#pragma once

#include <Scripting/IScript.h>

namespace NK
{

/**
 * Example C++ Script: PlayerController
 * 
 * This shows how to create a C++ script that can be attached to entities.
 * Scripts receive lifecycle callbacks and can access other components.
 */
class PlayerController : public IScript
{
public:
    // Called when script is first attached to an entity
    void OnStart() override
    {
        std::cout << "PlayerController: Started for entity " << m_entity << std::endl;
        
        // Get our transform component
        if (auto* transform = GetComponent<CTransform>())
        {
            m_initialPosition = transform->position;
        }
        
        // Get physics body if present
        if (auto* physics = GetComponent<CPhysicsBody>())
        {
            physics->SetLinearVelocity({0, 0, 0});
        }
        
        m_speed = 10.0f;
        m_jumpForce = 15.0f;
        m_isGrounded = false;
    }
    
    // Called every frame
    void OnUpdate(float deltaTime) override
    {
        auto* transform = GetComponent<CTransform>();
        if (!transform) return;
        
        glm::vec3 movement{0, 0, 0};
        
        // Example input handling (would use actual InputManager in real code)
        // if (InputManager::GetKeyPressed(KEYBOARD_KEY_W))
        // {
        //     movement.z += 1.0f;
        // }
        
        // Normalize and apply speed
        if (glm::length(movement) > 0)
        {
            movement = glm::normalize(movement) * m_speed * deltaTime;
            transform->position += movement;
        }
    }
    
    // Called at fixed intervals (for physics)
    void OnFixedUpdate(float fixedDeltaTime) override
    {
        // Physics-related updates
    }
    
    // Called after all updates (e.g., for camera follow)
    void OnLateUpdate(float deltaTime) override
    {
        // Camera follow logic
    }
    
    // Called when script is removed or entity is destroyed
    void OnDestroy() override
    {
        std::cout << "PlayerController: Destroyed for entity " << m_entity << std::endl;
        
        // Cleanup: save state, release resources, etc.
    }
    
    // Collision callbacks (requires physics system integration)
    void OnCollisionEnter(Entity other) override
    {
        std::cout << "PlayerController: Collided with entity " << other << std::endl;
        m_isGrounded = true;
    }
    
    void OnCollisionExit(Entity other) override
    {
        std::cout << "PlayerController: Exited collision with entity " << other << std::endl;
    }
    
    // Setters for exposed properties
    void SetSpeed(float speed) { m_speed = speed; }
    void SetJumpForce(float force) { m_jumpForce = force; }
    
private:
    float m_speed = 10.0f;
    float m_jumpForce = 15.0f;
    bool m_isGrounded = false;
    glm::vec3 m_initialPosition;
};

/**
 * Example C++ Script: Rotator
 * 
 * A simple script that rotates an entity continuously.
 */
class Rotator : public IScript
{
public:
    void OnUpdate(float deltaTime) override
    {
        auto* transform = GetComponent<CTransform>();
        if (!transform) return;
        
        transform->rotation.y += m_rotationSpeed * deltaTime;
    }
    
    void SetRotationSpeed(float speed) { m_rotationSpeed = speed; }
    
private:
    float m_rotationSpeed = 90.0f; // degrees per second
};

/**
 * Example C++ Script: FollowTarget
 * 
 * Makes an entity follow another entity (e.g., camera follow).
 */
class FollowTarget : public IScript
{
public:
    void SetTarget(Entity target) { m_target = target; }
    void SetOffset(const glm::vec3& offset) { m_offset = offset; }
    void SetSmoothTime(float time) { m_smoothTime = time; }
    
    void OnUpdate(float deltaTime) override
    {
        if (m_target == Entity::Invalid()) return;
        
        auto* myTransform = GetComponent<CTransform>();
        auto* targetTransform = GetComponent<CTransform>();
        if (!myTransform || !targetTransform) return;
        
        // Smooth follow
        glm::vec3 targetPos = targetTransform->position + m_offset;
        myTransform->position = glm::lerp(myTransform->position, targetPos, m_smoothTime * deltaTime);
    }
    
private:
    Entity m_target = Entity::Invalid();
    glm::vec3 m_offset{0, 5, -10};
    float m_smoothTime = 2.0f;
};

} // namespace NK