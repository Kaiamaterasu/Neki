# Improvements for Neki Game Engine

This document outlines potential improvements for the Neki game engine, categorized by area. Each improvement includes description, code snippets, and implementation guidance.

---

## Table of Contents
1. [Architecture & Design](#1-architecture--design)
2. [Performance](#2-performance)
3. [Code Quality](#3-code-quality)
4. [Missing Features](#4-missing-features)
5. [Tools & Editor](#5-tools--editor)
6. [Build & DevOps](#6-build--devops)
7. [Security](#7-security)

---

## 1. Architecture & Design

### 1.1 Scripting Layer
**Status**: 🔵 Planned | **Priority**: High

Add a scripting layer to allow faster iteration for game logic without recompiling C++.

**Implementation**:
```cpp
// Example: Scripting system interface
#pragma once
#include <string>
#include <memory>
#include <vector>

namespace NK {

// Base class for scriptable entities
class IScript {
public:
    virtual ~IScript() = default;
    virtual void OnStart() {}
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnDestroy() {}
};

// Script engine interface
class IScriptEngine {
public:
    virtual ~IScriptEngine() = default;
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual std::unique_ptr<IScript> LoadScript(const std::string& filepath) = 0;
    virtual void Update(float deltaTime) = 0;
};

// Script manager to handle all scripts
class ScriptManager {
public:
    void Initialize();
    void Update(float deltaTime);
    void Shutdown();
    
    template<typename T>
    T* AddScript(Entity entity) {
        auto script = std::make_unique<T>();
        T* ptr = script.get();
        m_scripts[entity].push_back(std::move(script));
        return ptr;
    }
    
private:
    std::unordered_map<Entity, std::vector<std::unique_ptr<IScript>>> m_scripts;
};

} // namespace NK
```

**Usage Example**:
```cpp
// In a game entity script
class PlayerController : public IScript {
public:
    void OnStart() override {
        m_speed = 5.0f;
    }
    
    void OnUpdate(float deltaTime) override {
        // Handle input and movement
        auto& transform = GetComponent<CTransform>(GetEntity());
        transform.position += GetInputDirection() * m_speed * deltaTime;
    }
    
private:
    float m_speed = 10.0f;
};

// Register in setup
scriptManager->AddScript<PlayerController>(playerEntity);
```

---

### 1.2 Scene Serialization
**Status**: 🔵 Planned | **Priority**: High

Add proper YAML/JSON scene serialization for editor integration and save/load functionality.

**Implementation**:
```cpp
// Example: Scene serialization interface
#pragma once
#include <string>
#include <fstream>
#include <Core-ECS/Registry.h>

namespace NK {

class SceneSerializer {
public:
    explicit SceneSerializer(Registry& registry) : m_registry(registry) {}
    
    bool Serialize(const std::string& filepath);
    bool Deserialize(const std::string& filepath);
    
private:
    Registry& m_registry;
    
    // Serialize helpers
    void SerializeEntity(Entity entity, YAML::Emitter& out);
    void SerializeTransform(CTransform& transform, YAML::Emitter& out);
    void SerializeModel(CModelRenderer& model, YAML::Emitter& out);
    
    // Deserialize helpers
    Entity DeserializeEntity(const YAML::Node& node);
    void DeserializeTransform(Entity entity, const YAML::Node& node);
    void DeserializeModel(Entity entity, const YAML::Node& node);
};

// Example output (YAML format):
/*
Scene:
  name: "Level1"
  entities:
    - id: 1
      components:
        Transform:
          position: [0, 0, 0]
          rotation: [0, 0, 0]
          scale: [1, 1, 1]
        ModelRenderer:
          modelPath: "assets/models/cube.nkmodel"
*/

} // namespace NK
```

---

### 1.3 Message Queue / Event Bus
**Status**: 🔵 Planned | **Priority**: Medium

Implement a decoupled event system for inter-system communication.

**Implementation**:
```cpp
// Example: Event bus system
#pragma once
#include <functional>
#include <unordered_map>
#include <vector>
#include <typeindex>

namespace NK {

// Event base class
struct Event {
    virtual ~Event() = default;
};

// Event listener interface
class IEventListener {
public:
    virtual ~IEventListener() = default;
    virtual void OnEvent(const Event& event) = 0;
};

// Event bus for publish-subscribe
class EventBus {
public:
    template<typename T>
    void Subscribe(IEventListener* listener) {
        auto type = std::type_index(typeid(T));
        m_listeners[type].push_back(listener);
    }
    
    template<typename T>
    void Unsubscribe(IEventListener* listener) {
        auto type = std::type_index(typeid(T));
        auto& listeners = m_listeners[type];
        listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
    }
    
    template<typename T, typename... Args>
    void Publish(Args&&... args) {
        T event(std::forward<Args>(args)...);
        auto type = std::type_index(typeid(T));
        
        for (auto* listener : m_listeners[type]) {
            listener->OnEvent(event);
        }
    }
    
private:
    std::unordered_map<std::type_index, std::vector<IEventListener*>> m_listeners;
};

// Example events
struct PlayerDiedEvent {
    Entity player;
    int score;
};

struct ScoreChangedEvent {
    Entity player;
    int newScore;
};

} // namespace NK
```

---

## 2. Performance

### 2.1 Multi-threaded Command Buffer Generation
**Status**: 🔵 Planned | **Priority**: Medium

Add parallel rendering command generation using thread pools.

**Implementation**:
```cpp
// Example: Parallel render preparation
#include <thread>
#include <future>
#include <vector>

namespace NK {

class ParallelRenderer {
public:
    void PrepareRenderData(std::vector<Entity>& entities) {
        const size_t numThreads = std::thread::hardware_concurrency();
        const size_t chunkSize = entities.size() / numThreads;
        
        std::vector<std::future<void>> futures;
        
        for (size_t i = 0; i < numThreads; ++i) {
            size_t start = i * chunkSize;
            size_t end = (i == numThreads - 1) ? entities.size() : (i + 1) * chunkSize;
            
            futures.push_back(std::async(std::launch::async, [this, &entities, start, end]() {
                PrepareEntityRange(entities.data() + start, end - start);
            }));
        }
        
        // Wait for all threads to complete
        for (auto& future : futures) {
            future.get();
        }
    }
    
private:
    void PrepareEntityRange(Entity* entities, size_t count);
};

} // namespace NK
```

---

### 2.2 Shader Permutation System
**Status**: 🔵 Planned | **Priority**: Medium

Reduce pipeline count by consolidating shader variants with #define injection.

**Implementation**:
```cpp
// Example: Shader permutation manager
#pragma once
#include <unordered_map>
#include <string>
#include <vector>

namespace NK {

struct ShaderVariant {
    std::string baseShader;
    std::vector<std::pair<std::string, std::string>> defines; // key, value
    VkPipeline pipeline = VK_NULL_HANDLE;
};

class ShaderPermutationManager {
public:
    // Register a shader with possible variants
    void RegisterBaseShader(const std::string& name, const std::vector<std::string>& possibleDefines);
    
    // Get or create a variant with specific defines
    VkPipeline GetPipeline(const std::string& name, const std::unordered_map<std::string, std::string>& activeDefines);
    
    // Example usage:
    // Register: "ModelPBR" with possible defines: {"ENABLE_SHADOWS", "ENABLE_FOG", "ENABLE_EMISSIVE"}
    // Get: {"ENABLE_SHADOWS": "1", "ENABLE_FOG": "0"} -> creates variant
    
private:
    std::unordered_map<std::string, ShaderVariant> m_variants;
};

} // namespace NK
```

---

## 3. Code Quality

### 3.1 Unit Tests
**Status**: 🔵 Planned | **Priority**: High

Add Google Test or Catch2 for unit testing.

**Implementation**:
```cpp
// Example: Test setup with CMake
/*
# In CMakeLists.txt
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

add_executable(NekiTests
    Tests/CoreTests.cpp
    Tests/ECSTests.cpp
    Tests/MathTests.cpp
)
target_link_libraries(NekiTests PRIVATE GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(NekiTests)
*/

// Example test
#include <gtest/gtest.h>
#include <glm/glm.hpp>

namespace NK {
namespace Tests {

TEST(MathTest, Vec3Operations) {
    glm::vec3 a(1.0f, 2.0f, 3.0f);
    glm::vec3 b(4.0f, 5.0f, 6.0f);
    
    glm::vec3 result = a + b;
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 7.0f);
    EXPECT_FLOAT_EQ(result.z, 9.0f);
}

TEST(ECSTest, EntityCreation) {
    Registry registry;
    auto entity = registry.CreateEntity();
    EXPECT_TRUE(registry.IsValid(entity));
    
    registry.AddComponent<CTransform>(entity);
    EXPECT_TRUE(registry.HasComponent<CTransform>(entity));
}

} // namespace Tests
} // namespace NK
```

---

### 3.2 Replace Raw Pointers with Smart Pointers
**Status**: 🔵 Planned | **Priority**: Medium

Gradually replace raw pointers with smart pointers for better memory safety.

**Before**:
```cpp
class ResourceManager {
    ITexture* LoadTexture(const std::string& path); // Raw pointer
    ITexture* m_defaultTexture = nullptr;
};
```

**After**:
```cpp
class ResourceManager {
    std::shared_ptr<ITexture> LoadTexture(const std::string& path);
    std::shared_ptr<ITexture> m_defaultTexture = nullptr;
    
    // For internal-only ownership
    std::unique_ptr<ITexture> m_internalTexture;
};
```

---

## 4. Missing Features

### 4.1 Audio System
**Status**: 🔵 Planned | **Priority**: High

Add audio engine for sound effects and music.

**Implementation**:
```cpp
// Example: Audio system interface
#pragma once
#include <string>
#include <vector>
#include <memory>

namespace NK {

class IAudioSource {
public:
    virtual ~IAudioSource() = default;
    virtual void Play() = 0;
    virtual void Stop() = 0;
    virtual void Pause() = 0;
    virtual void SetVolume(float volume) = 0;
    virtual void SetPosition(const glm::vec3& pos) = 0;
    virtual bool IsPlaying() const = 0;
};

class AudioEngine {
public:
    bool Initialize();
    void Shutdown();
    
    std::shared_ptr<IAudioSource> LoadSound(const std::string& filepath);
    std::shared_ptr<IAudioSource> PlaySound(const std::string& filepath, bool loop = false);
    
    // For 3D audio
    void SetListenerPosition(const glm::vec3& pos);
    void SetListenerOrientation(const glm::vec3& forward, const glm::vec3& up);
    
private:
    // Internal implementation (miniaudio, SDL_mixer, or custom)
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace NK
```

---

### 4.2 Animation System
**Status**: 🔵 Planned | **Priority**: High

Add skeletal animation with blending.

**Implementation**:
```cpp
// Example: Animation system
#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <Core-ECS/Entity.h>

namespace NK {

struct TransformKey {
    glm::vec3 position;
    glm::vec3 rotation; // as euler angles
    glm::vec3 scale;
    float time;
};

struct AnimationBone {
    std::string name;
    int parentIndex;
    std::vector<TransformKey> positionKeys;
    std::vector<TransformKey> rotationKeys;
    std::vector<TransformKey> scaleKeys;
};

struct Animation {
    std::string name;
    float duration;
    float ticksPerSecond;
    std::vector<AnimationBone> bones;
};

class AnimationPlayer {
public:
    void Play(const Animation& anim, bool loop = true);
    void Stop();
    void SetWeight(float weight); // for blending
    
    // Update bone matrices for GPU upload
    void Update(float deltaTime);
    const std::vector<glm::mat4>& GetBoneMatrices() const { return m_boneMatrices; }
    
private:
    const Animation* m_currentAnimation = nullptr;
    float m_currentTime = 0.0f;
    bool m_looping = false;
    std::vector<glm::mat4> m_boneMatrices;
};

} // namespace NK
```

---

## 5. Tools & Editor

### 5.1 In-Game Profiler
**Status**: 🔵 Planned | **Priority**: Medium

Add performance profiler for debugging.

**Implementation**:
```cpp
// Example: Profiler system
#pragma once
#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>

namespace NK {

class Profiler {
public:
    static Profiler& Get() {
        static Profiler instance;
        return instance;
    }
    
    class ScopedTimer {
    public:
        ScopedTimer(const char* name) : m_name(name) {
            m_start = std::chrono::high_resolution_clock::now();
        }
        ~ScopedTimer() {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start);
            Profiler::Get().EndSection(m_name, duration.count());
        }
    private:
        const char* m_name;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
    };
    
    void BeginFrame();
    void EndFrame();
    void EndSection(const char* name, long long microseconds);
    
    // Render ImGui visualization
    void RenderUI();
    
private:
    struct ProfileSection {
        std::string name;
        long long totalMicroseconds = 0;
        int callCount = 0;
    };
    
    std::vector<ProfileSection> m_sections;
    long long m_frameTotal = 0;
};

// Usage: SCOPED_TIMER("UpdatePhysics");
#define SCOPED_TIMER(name) Profiler::ScopedTimer _timer##__LINE__(name)

} // namespace NK
```

---

## 6. Build & DevOps

### 6.1 GitHub Actions CI/CD
**Status**: 🔵 Planned | **Priority**: Medium

Add automated builds and testing.

**Implementation**:
```yaml
# Example: .github/workflows/build.yml
name: Build and Test

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  build:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-22.04, windows-latest]
        preset: [Vulkan-Debug, D3D12-Debug]
    
    steps:
    - uses: actions/checkout@v4
    
    - name: Configure CMake
      run: cmake --preset "${{ matrix.preset }}"
    
    - name: Build
      run: cmake --build --preset "${{ matrix.preset }}"
    
    - name: Upload Artifacts
      uses: actions/upload-artifact@v4
      with:
        name: ${{ matrix.os }}-${{ matrix.preset }}
        path: build/

  test:
    runs-on: ubuntu-latest
    needs: build
    
    steps:
    - uses: actions/checkout@v4
    
    - name: Run Tests
      run: ctest --preset testing
```

---

## 7. Security

### 7.1 Complete Deferred Vulnerability Fixes
**Status**: 🔵 Planned | **Priority**: Medium

Complete VULN-007 (dependency checksums) and VULN-008 (thread safety).

**Implementation for VULN-007**:
```cmake
# Example: Add checksum verification
FetchContent_Declare(
    dxc_artifacts
    URL "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.8.2505.1/dxc_2025_07_14.zip"
    DOWNLOAD_NO_EXTRACT TRUE
)

# Verify SHA256 (add actual checksum)
file(DOWNLOAD 
    "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.8.2505.1/dxc_2025_07_14.zip"
    "${CMAKE_BINARY_DIR}/dxc.zip"
    EXPECTED_HASH SHA256=abc123...
    SHOW_PROGRESS
)
```

---

## Summary

| Category | Item | Priority | Status |
|-----------|------|----------|--------|
| Architecture | Scripting Layer | High | 🔵 Planned |
| Architecture | Scene Serialization | High | 🔵 Planned |
| Architecture | Message Queue | Medium | 🔵 Planned |
| Performance | Multi-threaded Rendering | Medium | 🔵 Planned |
| Performance | Shader Permutation | Medium | 🔵 Planned |
| Code Quality | Unit Tests | High | 🔵 Planned |
| Code Quality | Smart Pointers | Medium | 🔵 Planned |
| Features | Audio System | High | 🔵 Planned |
| Features | Animation System | High | 🔵 Planned |
| Tools | In-Game Profiler | Medium | 🔵 Planned |
| DevOps | CI/CD Pipeline | Medium | 🔵 Planned |
| Security | Dependency Checksums | Medium | 🔵 Planned |

---

*Document generated for Neki Game Engine*
*Last updated: April 2026*