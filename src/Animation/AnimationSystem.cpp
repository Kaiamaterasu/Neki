#include "Animation/AnimationSystem.h"

#include <iostream>
#include <algorithm>
#include <cmath>

namespace NK
{

// ============================================================================
// Skeleton Implementation
// ============================================================================

void Skeleton::AddBone(const std::string& name, int parentIndex)
{
    Bone bone;
    bone.name = name;
    bone.parentIndex = parentIndex;
    
    int index = static_cast<int>(m_bones.size());
    m_boneIndices[name] = index;
    m_bones.push_back(bone);
}

int Skeleton::FindBone(const std::string& name) const
{
    auto it = m_boneIndices.find(name);
    if (it != m_boneIndices.end())
    {
        return it->second;
    }
    return -1;
}

const Bone& Skeleton::GetBone(int index) const
{
    static Bone invalid;
    if (index >= 0 && index < static_cast<int>(m_bones.size()))
    {
        return m_bones[index];
    }
    return invalid;
}

Bone& Skeleton::GetBone(int index)
{
    static Bone invalid;
    if (index >= 0 && index < static_cast<int>(m_bones.size()))
    {
        return m_bones[index];
    }
    return invalid;
}

void Skeleton::CalculateBoneMatrices(std::vector<glm::mat4>& matrices)
{
    matrices.resize(m_bones.size());
    
    for (size_t i = 0; i < m_bones.size(); ++i)
    {
        Bone& bone = m_bones[i];
        
        // Calculate world transform
        if (bone.parentIndex >= 0 && bone.parentIndex < static_cast<int>(i))
        {
            bone.worldTransform = m_bones[bone.parentIndex].worldTransform * bone.localTransform;
        }
        else
        {
            bone.worldTransform = bone.localTransform;
        }
        
        matrices[i] = bone.worldTransform;
    }
}

// ============================================================================
// AnimationPlayer Implementation
// ============================================================================

AnimationPlayer::AnimationPlayer()
{
}

void AnimationPlayer::SetSkeleton(std::shared_ptr<Skeleton> skeleton)
{
    m_skeleton = skeleton;
    if (skeleton)
    {
        m_boneMatrices.resize(skeleton->GetBoneCount(), glm::mat4(1.0f));
    }
}

void AnimationPlayer::Play(const Animation& animation, bool loop)
{
    m_currentAnimation = &animation;
    m_currentTime = 0.0f;
    m_looping = loop;
    m_playing = true;
    m_paused = false;
    
    // Resize matrices if needed
    if (m_skeleton && m_skeleton->GetBoneCount() != static_cast<int>(m_boneMatrices.size()))
    {
        m_boneMatrices.resize(m_skeleton->GetBoneCount(), glm::mat4(1.0f));
    }
}

void AnimationPlayer::Stop()
{
    m_playing = false;
    m_currentTime = 0.0f;
}

void AnimationPlayer::Pause()
{
    m_paused = true;
}

void AnimationPlayer::Resume()
{
    m_paused = false;
}

void AnimationPlayer::SetWeight(float weight)
{
    m_weight = std::clamp(weight, 0.0f, 1.0f);
}

void AnimationPlayer::Update(float deltaTime)
{
    if (!m_playing || m_paused || !m_currentAnimation)
    {
        return;
    }
    
    // Advance time
    float timeScale = m_currentAnimation->ticksPerSecond / 30.0f; // Normalize to 30 fps
    m_currentTime += deltaTime * timeScale;
    
    // Handle looping
    if (m_currentTime >= m_currentAnimation->duration)
    {
        if (m_looping)
        {
            // Wrap time
            float duration = m_currentAnimation->duration;
            m_currentTime = std::fmod(m_currentTime, duration);
        }
        else
        {
            m_currentTime = m_currentAnimation->duration;
            m_playing = false;
        }
    }
    
    // Calculate bone transforms
    CalculateBoneTransforms();
}

void AnimationPlayer::CalculateBoneTransforms()
{
    if (!m_skeleton || !m_currentAnimation)
    {
        return;
    }
    
    const auto& bones = m_skeleton->GetBones();
    
    for (size_t i = 0; i < bones.size(); ++i)
    {
        const Bone& skelBone = bones[i];
        
        // Find corresponding bone in animation
        const Bone* animBone = nullptr;
        for (const auto& ab : m_currentAnimation->bones)
        {
            if (ab.name == skelBone.name)
            {
                animBone = &ab;
                break;
            }
        }
        
        if (!animBone)
        {
            // No animation for this bone, use default
            continue;
        }
        
        // Interpolate transform
        m_skeleton->GetBone(i).localTransform = InterpolateTransform(*animBone, m_currentTime);
    }
    
    // Calculate final matrices
    m_skeleton->CalculateBoneMatrices(m_boneMatrices);
}

glm::mat4 AnimationPlayer::InterpolateTransform(const Bone& bone, float time)
{
    glm::mat4 result = glm::mat4(1.0f);
    
    // Interpolate position
    glm::vec3 position = {0, 0, 0};
    if (!bone.positionKeys.empty())
    {
        position = FindKeyframe(bone.positionKeys, time, &TransformKey::position);
    }
    
    // Interpolate rotation
    glm::quat rotation = glm::quat_identity();
    if (!bone.rotationKeys.empty())
    {
        glm::vec3 euler = FindKeyframe(bone.rotationKeys, time, &TransformKey::rotation);
        rotation = glm::quat(glm::radians(euler));
    }
    
    // Interpolate scale
    glm::vec3 scale = {1, 1, 1};
    if (!bone.scaleKeys.empty())
    {
        scale = FindKeyframe(bone.scaleKeys, time, &TransformKey::scale);
    }
    
    // Compose transform
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
    glm::mat4 scaling = glm::scale(glm::mat4(1.0f), scale);
    
    return translation * rotationMatrix * scaling;
}

template<typename T, typename Member>
T AnimationPlayer::FindKeyframe(const std::vector<TransformKey>& keys, float time, Member member)
{
    if (keys.empty())
    {
        return T{};
    }
    
    if (keys.size() == 1)
    {
        return keys[0].*member;
    }
    
    // Find surrounding keyframes
    int beforeIndex = -1;
    int afterIndex = -1;
    
    for (size_t i = 0; i < keys.size(); ++i)
    {
        if (keys[i].time <= time)
        {
            beforeIndex = static_cast<int>(i);
        }
        if (keys[i].time >= time && afterIndex == -1)
        {
            afterIndex = static_cast<int>(i);
        }
    }
    
    // Handle edge cases
    if (beforeIndex == -1)
    {
        return keys[0].*member;
    }
    if (afterIndex == -1 || afterIndex == beforeIndex)
    {
        return keys[beforeIndex].*member;
    }
    
    // Interpolate between keyframes
    const TransformKey& before = keys[beforeIndex];
    const TransformKey& after = keys[afterIndex];
    
    float t = (time - before.time) / (after.time - before.time);
    t = std::clamp(t, 0.0f, 1.0f);
    
    if (before.interpolation == TransformKey::Interpolation::Step)
    {
        return before.*member;
    }
    
    // Linear interpolation
    return Lerp(before.*member, after.*member, t);
}

glm::vec3 AnimationPlayer::Lerp(const glm::vec3& a, const glm::vec3& b, float t)
{
    return a + (b - a) * t;
}

glm::quat AnimationPlayer::Slerp(const glm::quat& a, const glm::quat& b, float t)
{
    float dot = glm::dot(a, b);
    
    // Ensure shortest path
    glm::quat bias = b;
    if (dot < 0)
    {
        bias = -b;
        dot = -dot;
    }
    
    if (dot > 0.9995f)
    {
        // Use lerp for near-parallel quaternions
        return glm::normalize(a + (bias - a) * t);
    }
    
    float theta0 = std::acos(dot);
    float theta = theta0 * t;
    float sinTheta = std::sin(theta);
    float sinTheta0 = std::sin(theta0);
    
    float s0 = std::cos(theta) - dot * sinTheta / sinTheta0;
    float s1 = sinTheta / sinTheta0;
    
    return glm::normalize(a * s0 + bias * s1);
}

// ============================================================================
// AnimationManager Implementation
// ============================================================================

AnimationManager::AnimationManager()
{
}

AnimationManager::~AnimationManager()
{
    Shutdown();
}

bool AnimationManager::Initialize()
{
    if (m_initialized)
    {
        return true;
    }
    
    std::cout << "AnimationManager: Initialized" << std::endl;
    m_initialized = true;
    return true;
}

void AnimationManager::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }
    
    m_activePlayers.clear();
    m_initialized = false;
    std::cout << "AnimationManager: Shutdown complete" << std::endl;
}

std::pair<std::shared_ptr<Skeleton>, std::vector<Animation>> AnimationManager::LoadFromGLTF(const std::string& filepath)
{
    // In production, use a GLTF loader like cgltf or tinygltf
    // This is a placeholder showing the interface
    
    std::cout << "AnimationManager: (Stub) Would load GLTF: " << filepath << std::endl;
    
    auto skeleton = std::make_shared<Skeleton>();
    std::vector<Animation> animations;
    
    return {skeleton, animations};
}

std::unique_ptr<Animation> AnimationManager::LoadAnimation(const std::string& filepath)
{
    // Load animation from BVH or other format
    std::cout << "AnimationManager: (Stub) Would load animation: " << filepath << std::endl;
    return nullptr;
}

std::shared_ptr<AnimationPlayer> AnimationManager::CreatePlayer(std::shared_ptr<Skeleton> skeleton)
{
    auto player = std::make_shared<AnimationPlayer>();
    player->SetSkeleton(skeleton);
    m_activePlayers.push_back(player);
    return player;
}

void AnimationManager::Update(float deltaTime)
{
    for (auto& player : m_activePlayers)
    {
        if (player->IsPlaying())
        {
            player->Update(deltaTime);
        }
    }
}

} // namespace NK