#include "Audio/AudioEngine.h"

#include <iostream>
#include <algorithm>

namespace NK
{

// ============================================================================
// AudioListener Implementation
// ============================================================================

AudioListener::AudioListener()
{
    // Initialize OpenAL listener to defaults
    alListenerfv(AL_POSITION, glm::value_ptr(m_position));
    alListenerfv(AL_ORIENTATION, glm::value_ptr(m_forward));
    
    // Up vector is second half of orientation
    float orientation[6] = {
        m_forward.x, m_forward.y, m_forward.z,
        m_up.x, m_up.y, m_up.z
    };
    alListenerfv(AL_ORIENTATION, orientation);
}

AudioListener::~AudioListener()
{
}

void AudioListener::SetPosition(const glm::vec3& position)
{
    m_position = position;
    alListenerfv(AL_POSITION, glm::value_ptr(m_position));
}

void AudioListener::SetOrientation(const glm::vec3& forward, const glm::vec3& up)
{
    m_forward = forward;
    m_up = up;
    
    float orientation[6] = {
        m_forward.x, m_forward.y, m_forward.z,
        m_up.x, m_up.y, m_up.z
    };
    alListenerfv(AL_ORIENTATION, orientation);
}

void AudioListener::SetMasterVolume(float volume)
{
    m_masterVolume = std::clamp(volume, 0.0f, 1.0f);
    alListenerf(AL_GAIN, m_masterVolume);
}

void AudioListener::Update()
{
    // Update listener position/orientation if needed
    // This is called each frame to ensure proper 3D audio
}

// ============================================================================
// AudioEngine Implementation
// ============================================================================

AudioEngine* AudioEngine::s_instance = nullptr;

AudioEngine::AudioEngine()
{
    s_instance = this;
}

AudioEngine::~AudioEngine()
{
    Shutdown();
    
    if (s_instance == this)
    {
        s_instance = nullptr;
    }
}

bool AudioEngine::Initialize()
{
    if (m_initialized)
    {
        std::cout << "AudioEngine: Already initialized" << std::endl;
        return true;
    }
    
    std::cout << "AudioEngine: Initializing..." << std::endl;
    
    // Open default audio device
    m_device = alcOpenDevice(nullptr);
    if (!m_device)
    {
        std::cerr << "AudioEngine: Failed to open audio device" << std::endl;
        return false;
    }
    
    // Create audio context
    m_context = alcCreateContext(m_device, nullptr);
    if (!m_context)
    {
        std::cerr << "AudioEngine: Failed to create audio context" << std::endl;
        alcCloseDevice(m_device);
        m_device = nullptr;
        return false;
    }
    
    // Make context current
    if (!alcMakeContextCurrent(m_context))
    {
        std::cerr << "AudioEngine: Failed to make context current" << std::endl;
        alcDestroyContext(m_context);
        alcCloseDevice(m_device);
        m_context = nullptr;
        m_device = nullptr;
        return false;
    }
    
    // Set distance model
    SetDistanceModel(m_distanceModel);
    SetRolloffFactor(m_rolloffFactor);
    SetReferenceDistance(m_referenceDistance);
    SetMaxDistance(m_maxDistance);
    
    m_initialized = true;
    std::cout << "AudioEngine: Initialized successfully" << std::endl;
    return true;
}

void AudioEngine::Shutdown()
{
    if (!m_initialized) return;
    
    std::cout << "AudioEngine: Shutting down..." << std::endl;
    
    // Stop all active sources
    m_activeSources.clear();
    
    // Delete all buffers
    for (auto& [name, buffer] : m_buffers)
    {
        alDeleteBuffers(1, &buffer);
    }
    m_buffers.clear();
    
    // Destroy context and close device
    if (m_context)
    {
        alcDestroyContext(m_context);
        m_context = nullptr;
    }
    
    if (m_device)
    {
        alcCloseDevice(m_device);
        m_device = nullptr;
    }
    
    m_initialized = false;
    std::cout << "AudioEngine: Shutdown complete" << std::endl;
}

void AudioEngine::Update()
{
    if (!m_initialized) return;
    
    // Clean up expired weak references
    m_activeSources.erase(
        std::remove_if(m_activeSources.begin(), m_activeSources.end(),
            [](const std::weak_ptr<IAudioSource>& wp) { return wp.expired(); }),
        m_activeSources.end()
    );
}

std::shared_ptr<IAudioSource> AudioEngine::LoadSound(const std::string& filepath)
{
    if (!m_initialized)
    {
        std::cerr << "AudioEngine: Cannot load sound - not initialized" << std::endl;
        return nullptr;
    }
    
    // Check if already loaded
    auto it = m_buffers.find(filepath);
    if (it != m_buffers.end())
    {
        // Return new source using existing buffer
        // Would create OpenAL source wrapper here
        std::cout << "AudioEngine: Using cached buffer for " << filepath << std::endl;
    }
    
    // Load new file
    ALuint buffer = LoadAudioFile(filepath);
    if (buffer == 0)
    {
        std::cerr << "AudioEngine: Failed to load sound: " << filepath << std::endl;
        return nullptr;
    }
    
    m_buffers[filepath] = buffer;
    
    // Create and return source
    // In actual implementation, would return IAudioSource implementation
    std::cout << "AudioEngine: Loaded sound: " << filepath << std::endl;
    return nullptr;
}

std::shared_ptr<IAudioSource> AudioEngine::LoadMusic(const std::string& filepath)
{
    // Music is similar to sound but typically streamed
    // For now, treat same as sound
    return LoadSound(filepath);
}

void AudioEngine::PlaySound(const std::string& filepath, float volume, bool loop)
{
    auto sound = LoadSound(filepath);
    if (sound)
    {
        sound->SetVolume(volume);
        sound->SetLooping(loop);
        sound->Play();
    }
}

std::shared_ptr<IAudioSource> AudioEngine::PlayMusic(const std::string& filepath, bool loop)
{
    auto music = LoadMusic(filepath);
    if (music)
    {
        music->SetLooping(loop);
        music->Play();
    }
    return music;
}

std::unique_ptr<AudioEngine::Stream> AudioEngine::CreateStream(const std::string& filepath)
{
    // Would implement streaming source
    return nullptr;
}

void AudioEngine::SetDistanceModel(DistanceModel model)
{
    m_distanceModel = model;
    
    if (!m_initialized) return;
    
    switch (model)
    {
        case DistanceModel::None:
            alDistanceModel(AL_NONE);
            break;
        case DistanceModel::Linear:
            alDistanceModel(AL_LINEAR_DISTANCE);
            break;
        case DistanceModel::Exponential:
            alDistanceModel(AL_EXPONENT_DISTANCE);
            break;
    }
}

void AudioEngine::SetRolloffFactor(float factor)
{
    m_rolloffFactor = factor;
    if (m_initialized)
    {
        alfFloat(AL_AIR_ABSORPTION_FACTOR, factor);
    }
}

void AudioEngine::SetReferenceDistance(float distance)
{
    m_referenceDistance = distance;
    if (m_initialized)
    {
        alfFloat(AL_REFERENCE_DISTANCE, distance);
    }
}

void AudioEngine::SetMaxDistance(float distance)
{
    m_maxDistance = distance;
    if (m_initialized)
    {
        alfFloat(AL_MAX_DISTANCE, distance);
    }
}

ALuint AudioEngine::LoadAudioFile(const std::string& filepath)
{
    // This would use a library like stb_vorbis or miniaudio to decode audio
    // For now, return 0 (failure)
    
    // Example of what would be needed:
    // stb_vorbis* vorbis = stb_vorbis_open_filename(filepath.c_str(), nullptr, nullptr);
    // if (vorbis) { ... decode and create AL buffer ... }
    
    std::cout << "AudioEngine: (Stub) Would load: " << filepath << std::endl;
    return 0;
}

} // namespace NK