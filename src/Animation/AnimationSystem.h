#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <memory>

#include <Core-ECS/Entity.h>

namespace NK
{

// ============================================================================
// Animation Data Structures
// ============================================================================

/**
 * TransformKey - A keyframe for transform animation
 */
struct TransformKey
{
    glm::vec3 position{0, 0, 0};
    glm::vec3 rotation{0, 0, 0};  // Euler angles in degrees
    glm::vec3 scale{1, 1, 1};
    float time = 0.0f;
    
    // Interpolation type
    enum class Interpolation
    {
        Step,      // No interpolation
        Linear,    // Linear interpolation
        Cubic      // Cubic spline interpolation
    };
    Interpolation interpolation = Interpolation::Linear;
};

/**
 * FloatKey - Simple float keyframe (for material properties, etc.)
 */
struct FloatKey
{
    float value = 0.0f;
    float time = 0.0f;
};

/**
 * Bone - Animation bone with keyframes
 */
struct Bone
{
    std::string name;
    int parentIndex = -1;          // -1 for root
    
    std::vector<TransformKey> positionKeys;
    std::vector<TransformKey> rotationKeys;
    std::vector<TransformKey> scaleKeys;
    
    // Local transform
    glm::mat4 localTransform = glm::mat4(1.0f);
    
    // Computed world transform
    glm::mat4 worldTransform = glm::mat4(1.0f);
};

/**
 * Animation - A single animation (walk, run, idle, etc.)
 */
struct Animation
{
    std::string name;
    float duration = 0.0f;        // Total duration in seconds
    float ticksPerSecond = 30.0f;  // Animation speed
    
    std::vector<Bone> bones;
    
    /**
     * Get animation time in seconds from a 0-1 progress value
     */
    float GetTimeFromProgress(float progress) const
    {
        return progress * duration;
    }
};

/**
 * Skeleton - Collection of bones forming a skeleton
 */
class Skeleton
{
public:
    Skeleton() = default;
    
    void AddBone(const std::string& name, int parentIndex = -1);
    int FindBone(const std::string& name) const;
    const Bone& GetBone(int index) const;
    Bone& GetBone(int index);
    
    const std::vector<Bone>& GetBones() const { return m_bones; }
    std::vector<Bone>& GetBones() { return m_bones; }
    
    int GetBoneCount() const { return static_cast<int>(m_bones.size()); }
    
    /**
     * Build skeleton hierarchy transforms
     */
    void CalculateBoneMatrices(std::vector<glm::mat4>& matrices);

private:
    std::vector<Bone> m_bones;
    std::unordered_map<std::string, int> m_boneIndices;
};

// ============================================================================
// Animation Player
// ============================================================================

/**
 * AnimationPlayer - Plays animations on an entity
 * 
 * Usage:
 *   AnimationPlayer player;
 *   player.SetSkeleton(skeleton);
 *   player.Play(animation, true); // loop
 *   
 *   void Update(float dt) {
 *     player.Update(dt);
 *     auto& matrices = player.GetBoneMatrices();
 *     // Upload matrices to GPU
 *   }
 */
class AnimationPlayer
{
public:
    AnimationPlayer();
    ~AnimationPlayer() = default;
    
    /**
     * Set the skeleton to animate
     */
    void SetSkeleton(std::shared_ptr<Skeleton> skeleton);
    std::shared_ptr<Skeleton> GetSkeleton() const { return m_skeleton; }
    
    /**
     * Play an animation
     */
    void Play(const Animation& animation, bool loop = true);
    
    /**
     * Stop current animation
     */
    void Stop();
    
    /**
     * Pause animation
     */
    void Pause();
    
    /**
     * Resume animation
     */
    void Resume();
    
    /**
     * Set animation blend weight (for blending between animations)
     */
    void SetWeight(float weight);
    float GetWeight() const { return m_weight; }
    
    /**
     * Update animation (call each frame)
     */
    void Update(float deltaTime);
    
    /**
     * Get computed bone matrices (for GPU upload)
     */
    const std::vector<glm::mat4>& GetBoneMatrices() const { return m_boneMatrices; }
    
    /**
     * Check if animation is playing
     */
    bool IsPlaying() const { return m_playing && !m_paused; }
    bool IsPaused() const { return m_paused; }
    
    /**
     * Get current animation time
     */
    float GetCurrentTime() const { return m_currentTime; }
    
    /**
     * Get animation progress (0-1)
     */
    float GetProgress() const
    {
        if (!m_currentAnimation || m_currentAnimation->duration <= 0) return 0;
        return m_currentTime / m_currentAnimation->duration;
    }

private:
    /**
     * Calculate bone transforms for current animation time
     */
    void CalculateBoneTransforms();

    /**
     * Interpolate between transform keys
     */
    glm::mat4 InterpolateTransform(const Bone& bone, float time);

    /**
     * Linear interpolation helper
     */
    glm::vec3 Lerp(const glm::vec3& a, const glm::vec3& b, float t);
    glm::quat Slerp(const glm::quat& a, const glm::quat& b, float t);

    std::shared_ptr<Skeleton> m_skeleton;
    const Animation* m_currentAnimation = nullptr;
    
    float m_currentTime = 0.0f;
    float m_weight = 1.0f;
    bool m_playing = false;
    bool m_paused = false;
    bool m_looping = false;
    
    std::vector<glm::mat4> m_boneMatrices;
};

// ============================================================================
// Animation Manager
// ============================================================================

/**
 * AnimationManager - Load and manage all animations
 * 
 * Usage:
 *   AnimationManager mgr;
 *   mgr.Initialize();
 *   
 *   // Load animation file (GLTF/FBX)
 *   auto [skeleton, animations] = mgr.LoadFromGLTF("character.glb");
 *   
 *   // Create player
 *   auto player = mgr.CreatePlayer(skeleton);
 *   player->Play(animations["idle"]);
 */
class AnimationManager
{
public:
    AnimationManager();
    ~AnimationManager();
    
    bool Initialize();
    void Shutdown();
    
    /**
     * Load skeleton and animations from GLTF file
     * Returns: pair<shared_ptr<Skeleton>, vector of Animations>
     */
    std::pair<std::shared_ptr<Skeleton>, std::vector<Animation>> LoadFromGLTF(const std::string& filepath);
    
    /**
     * Load animation from file (BVA, GLTF animation track)
     */
    std::unique_ptr<Animation> LoadAnimation(const std::string& filepath);
    
    /**
     * Create animation player for an entity
     */
    std::shared_ptr<AnimationPlayer> CreatePlayer(std::shared_ptr<Skeleton> skeleton);
    
    /**
     * Update all active animation players
     */
    void Update(float deltaTime);

private:
    bool m_initialized = false;
    std::vector<std::shared_ptr<AnimationPlayer>> m_activePlayers;
};

// ============================================================================
// Skeleton Builder (helper to create skeletons programmatically)
// ============================================================================

class SkeletonBuilder
{
public:
    SkeletonBuilder& AddBone(const std::string& name, const std::string& parent = "");
    SkeletonBuilder& AddBone(int parentIndex);
    
    std::shared_ptr<Skeleton> Build();
    
private:
    std::vector<Bone> m_bones;
    std::unordered_map<std::string, int> m_nameToIndex;
};

/*
// Example: Creating a simple skeleton programmatically

auto skeleton = std::make_shared<Skeleton>();

// Root bone (hip)
skeleton->AddBone("Hip", -1);

// Child bones
skeleton->AddBone("Spine", 0);      // Parent is bone 0 (Hip)
skeleton->AddBone("Head", 1);       // Parent is bone 1 (Spine)

skeleton->AddBone("LeftLeg", 0);
skeleton->AddBone("LeftFoot", 3);

skeleton->AddBone("RightLeg", 0);
skeleton->AddBone("RightFoot", 5);

skeleton->AddBone("LeftArm", 1);
skeleton->AddBone("LeftHand", 7);

skeleton->AddBone("RightArm", 1);
skeleton->AddBone("RightHand", 9);

// Example: Creating animation

Animation walkAnim;
walkAnim.name = "Walk";
walkAnim.duration = 1.0f; // 1 second
walkAnim.ticksPerSecond = 30.0f;
walkAnim.bones = skeleton->GetBones();

// Add keyframes to hip bone
TransformKey key1;
key1.time = 0.0f;
key1.position = {0, 1, 0};
walkAnim.bones[0].positionKeys.push_back(key1);

TransformKey key2;
key2.time = 0.5f;
key2.position = {0, 1.1f, 0}; // Slight up motion
walkAnim.bones[0].positionKeys.push_back(key2);

TransformKey key3;
key3.time = 1.0f;
key3.position = {0, 1, 0};
walkAnim.bones[0].positionKeys.push_back(key3);

// Play animation
AnimationPlayer player;
player.SetSkeleton(skeleton);
player.Play(walkAnim, true); // loop

// In update
void Update(float dt) {
    player.Update(dt);
    
    const auto& matrices = player.GetBoneMatrices();
    // Upload to GPU via uniform buffer or texture
}
*/

} // namespace NK