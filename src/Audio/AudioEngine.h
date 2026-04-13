#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <AL/al.h>
#include <AL/alc.h>

// Note: In production, use miniaudio or OpenAL
// This header defines the audio API for Neki

namespace NK
{

// ============================================================================
// Audio Types
// ============================================================================

/**
 * AudioSource - Represents a playable audio source (sound or music)
 */
class IAudioSource
{
public:
    virtual ~IAudioSource() = default;
    
    // Playback control
    virtual void Play() = 0;
    virtual void Stop() = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    
    // Looping
    virtual void SetLooping(bool loop) = 0;
    virtual bool IsLooping() const = 0;
    
    // Volume (0.0 to 1.0)
    virtual void SetVolume(float volume) = 0;
    virtual float GetVolume() const = 0;
    
    // Pitch (0.5 to 2.0)
    virtual void SetPitch(float pitch) = 0;
    virtual float GetPitch() const = 0;
    
    // Position (for 3D audio)
    virtual void SetPosition(const glm::vec3& position) = 0;
    virtual glm::vec3 GetPosition() const = 0;
    
    // State
    virtual bool IsPlaying() const = 0;
    virtual bool IsPaused() const = 0;
    virtual bool IsStopped() const = 0;
    
    // Get duration in seconds
    virtual float GetDuration() const = 0;
    
    // Get current playback position
    virtual float GetCurrentTime() const = 0;
    virtual void SetCurrentTime(float time) = 0;
};

/**
 * AudioListener - Represents the listener (player ears)
 */
class AudioListener
{
public:
    AudioListener();
    ~AudioListener();
    
    // Position and orientation
    void SetPosition(const glm::vec3& position);
    glm::vec3 GetPosition() const { return m_position; }
    
    void SetOrientation(const glm::vec3& forward, const glm::vec3& up);
    glm::vec3 GetForward() const { return m_forward; }
    glm::vec3 GetUp() const { return m_up; }
    
    // Master volume
    void SetMasterVolume(float volume);
    float GetMasterVolume() const { return m_masterVolume; }
    
    // Update OpenAL listener
    void Update();

private:
    glm::vec3 m_position{0, 0, 0};
    glm::vec3 m_forward{0, 0, -1};
    glm::vec3 m_up{0, 1, 0};
    float m_masterVolume = 1.0f;
};

// ============================================================================
// Audio Engine
// ============================================================================

/**
 * AudioEngine - Main audio system
 * 
 * Usage:
 *   AudioEngine audio;
 *   audio.Initialize();
 *   
 *   // Load sounds
 *   auto sound = audio.LoadSound("shoot.wav");
 *   auto music = audio.LoadMusic("background.mp3");
 *   
 *   // Play
 *   sound->Play();
 *   music->SetLooping(true);
 *   music->Play();
 *   
 *   // Update each frame
 *   audio.Update();
 *   
 *   // Cleanup
 *   audio.Shutdown();
 */
class AudioEngine
{
public:
    AudioEngine();
    ~AudioEngine();
    
    // Disable copying
    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;
    
    /**
     * Initialize the audio engine
     */
    bool Initialize();
    
    /**
     * Shutdown the audio engine
     */
    void Shutdown();
    
    /**
     * Update audio system (call each frame)
     */
    void Update();
    
    // =========================================================================
    // Sound Loading
    // =========================================================================
    
    /**
     * Load a sound effect (non-streaming, good for short sounds)
     */
    std::shared_ptr<IAudioSource> LoadSound(const std::string& filepath);
    
    /**
     * Load music (streaming, good for longer audio)
     */
    std::shared_ptr<IAudioSource> LoadMusic(const std::string& filepath);
    
    /**
     * Play a sound immediately (fire-and-forget)
     */
    void PlaySound(const std::string& filepath, float volume = 1.0f, bool loop = false);
    
    /**
     * Play music immediately
     */
    std::shared_ptr<IAudioSource> PlayMusic(const std::string& filepath, bool loop = true);
    
    // =========================================================================
    // Streaming
    // =========================================================================
    
    /**
     * Stream audio (for very long sounds or real-time audio)
     */
    class Stream
    {
    public:
        virtual ~Stream() = default;
        virtual void Update() = 0;
        virtual void Play() = 0;
        virtual void Stop() = 0;
        virtual void SetVolume(float volume) = 0;
    };
    
    /**
     * Create a streaming source
     */
    std::unique_ptr<Stream> CreateStream(const std::string& filepath);
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * Set the distance model for 3D audio
     */
    enum class DistanceModel
    {
        None,       // No distance attenuation
        Linear,     // Linear attenuation
        Exponential // Exponential attenuation
    };
    
    void SetDistanceModel(DistanceModel model);
    DistanceModel GetDistanceModel() const { return m_distanceModel; }
    
    /**
     * Set distance rolloff factor
     */
    void SetRolloffFactor(float factor);
    float GetRolloffFactor() const { return m_rolloffFactor; }
    
    /**
     * Set reference distance (where volume starts to decrease)
     */
    void SetReferenceDistance(float distance);
    float GetReferenceDistance() const { return m_referenceDistance; }
    
    /**
     * Set max distance (where volume stops decreasing)
     */
    void SetMaxDistance(float distance);
    float GetMaxDistance() const { return m_maxDistance; }
    
    // =========================================================================
    // State
    // =========================================================================
    
    bool IsInitialized() const { return m_initialized; }
    
    /**
     * Get global audio engine instance
     */
    static AudioEngine* Get() { return s_instance; }

private:
    bool m_initialized = false;
    ALCdevice* m_device = nullptr;
    ALCcontext* m_context = nullptr;
    
    // Loaded audio buffers
    std::unordered_map<std::string, ALuint> m_buffers;
    
    // Active sources (for cleanup)
    std::vector<std::weak_ptr<IAudioSource>> m_activeSources;
    
    // Configuration
    DistanceModel m_distanceModel = DistanceModel::Linear;
    float m_rolloffFactor = 1.0f;
    float m_referenceDistance = 1.0f;
    float m_maxDistance = 100.0f;
    
    // Singleton
    static AudioEngine* s_instance;
    
    // Helper to load audio file (WAV, OGG, FLAC support via stb_vorbis, etc.)
    ALuint LoadAudioFile(const std::string& filepath);
};

// ============================================================================
// Implementation would use miniaudio or similar
// ============================================================================

/*
Example usage:

// Initialize
AudioEngine::Get()->Initialize();

// Load and play sound
auto shootSound = AudioEngine::Get()->LoadSound("assets/sounds/shoot.wav");
shootSound->SetVolume(0.8f);
shootSound->Play();

// Load and play music  
auto music = AudioEngine::Get()->LoadMusic("assets/music/background.mp3");
music->SetLooping(true);
music->SetVolume(0.5f);
music->Play();

// In update loop
AudioEngine::Get()->Update();

// Cleanup
AudioEngine::Get()->Shutdown();

// 3D Audio Example
auto spatialSound = AudioEngine::Get()->LoadSound("assets/sounds/footsteps.wav");
spatialSound->SetPosition({10, 0, 5}); // 3D positioned
spatialSound->Play();

// Listener is at origin facing -Z
AudioListener::Get().SetPosition({0, 0, 0});
AudioListener::Get().SetOrientation({0, 0, -1}, {0, 1, 0});
*/

} // namespace NK