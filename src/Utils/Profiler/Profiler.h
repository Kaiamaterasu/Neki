#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <iostream>

// Forward declarations
class Profiler;

#if defined(PROFILER_ENABLED)

// Easy-to-use macro for profiling
#define PROFILE_SCOPE(name) ::Profiler::ScopedTimer _profileTimer##__LINE__(name)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)

#else

// Empty macros when profiling is disabled
#define PROFILE_SCOPE(name) ((void)0)
#define PROFILE_FUNCTION() ((void)0)

#endif

namespace NK
{

/**
 * Profiler - In-game performance profiling system
 * 
 * Usage:
 *   // At start of frame
 *   Profiler::Get().BeginFrame();
 *   
 *   // In code sections you want to profile
 *   {
 *       PROFILE_SCOPE("Physics Update");
 *       // ... physics code ...
 *   }
 *   
 *   // At end of frame
 *   Profiler::Get().EndFrame();
 *   
 *   // Render UI
 *   Profiler::Get().RenderUI();
 * 
 * Enable by defining PROFILER_ENABLED in your build
 */
class Profiler
{
public:
    /**
     * Get singleton instance
     */
    static Profiler& Get()
    {
        static Profiler instance;
        return instance;
    }
    
    /**
     * Scoped timer - automatically records time when scope exits
     */
    class ScopedTimer
    {
    public:
        explicit ScopedTimer(const char* name)
            : m_name(name)
        {
            m_start = std::chrono::high_resolution_clock::now();
        }
        
        ~ScopedTimer()
        {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start);
            Profiler::Get().EndSection(m_name, duration.count());
        }
        
    private:
        const char* m_name;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
    };
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }
    
    // Maximum number of frames to average over
    void SetFrameAveraging(int frames) { m_frameAveraging = frames; }
    int GetFrameAveraging() const { return m_frameAveraging; }
    
    // =========================================================================
    // Frame Management
    // =========================================================================
    
    /**
     * Begin a new profiling frame
     */
    void BeginFrame()
    {
        if (!m_enabled) return;
        
        m_frameStart = std::chrono::high_resolution_clock::now();
        m_frameSections.clear();
    }
    
    /**
     * End the current frame
     */
    void EndFrame()
    {
        if (!m_enabled) return;
        
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - m_frameStart);
        m_frameTotalMicroseconds = frameDuration.count();
        
        // Add frame total as a section
        EndSection("Frame Total", m_frameTotalMicroseconds);
        
        // Update running averages
        UpdateAverages();
    }
    
    /**
     * Record a profiling section (called by ScopedTimer)
     */
    void EndSection(const char* name, long long microseconds)
    {
        if (!m_enabled) return;
        
        m_frameSections[name] = microseconds;
    }
    
    // =========================================================================
    // Statistics
    // =========================================================================
    
    struct SectionStats
    {
        std::string name;
        long long currentMicroseconds = 0;
        long long averageMicroseconds = 0;
        long long minMicroseconds = 0;
        long long maxMicroseconds = 0;
        double percentage = 0.0;
        int callCount = 0;
    };
    
    /**
     * Get all section statistics
     */
    std::vector<SectionStats> GetSectionStats() const
    {
        std::vector<SectionStats> stats;
        
        for (const auto& [name, data] : m_runningAverages)
        {
            SectionStats stat;
            stat.name = name;
            stat.currentMicroseconds = data.current;
            stat.averageMicroseconds = data.average;
            stat.minMicroseconds = data.min;
            stat.maxMicroseconds = data.max;
            stat.callCount = data.callCount;
            stat.percentage = (m_frameTotalMicroseconds > 0) 
                ? (100.0 * data.average / m_frameTotalMicroseconds) 
                : 0.0;
            
            stats.push_back(stat);
        }
        
        // Sort by average time (slowest first)
        std::sort(stats.begin(), stats.end(), 
            [](const SectionStats& a, const SectionStats& b) {
                return a.averageMicroseconds > b.averageMicroseconds;
            });
        
        return stats;
    }
    
    /**
     * Get current frame time in milliseconds
     */
    double GetFrameTimeMs() const
    {
        return m_frameTotalMicroseconds / 1000.0;
    }
    
    /**
     * Get current FPS
     */
    double GetFPS() const
    {
        if (m_frameTotalMicroseconds == 0) return 0.0;
        return 1000000.0 / m_frameTotalMicroseconds;
    }
    
    // =========================================================================
    // ImGui Visualization
    // =========================================================================
    
    /**
     * Render profiler UI (call from ImGui rendering)
     */
    void RenderUI()
    {
        #ifdef IMGUI_ENABLED
        if (ImGui::Begin("Profiler"))
        {
            // FPS and frame time
            ImGui::Text("FPS: %.1f", GetFPS());
            ImGui::Text("Frame Time: %.2f ms", GetFrameTimeMs());
            ImGui::Separator();
            
            // Section breakdown
            if (ImGui::BeginTable("Sections", 5))
            {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Current", ImGuiTableColumnFlags_WidthFixed, 80);
                ImGui::TableSetupColumn("Average", ImGuiTableColumnFlags_WidthFixed, 80);
                ImGui::TableSetupColumn("Min/Max", ImGuiTableColumnFlags_WidthFixed, 100);
                ImGui::TableSetupColumn("Percent", ImGuiTableColumnFlags_WidthFixed, 80);
                ImGui::TableHeadersRow();
                
                for (const auto& stat : GetSectionStats())
                {
                    if (stat.name == "Frame Total") continue;
                    
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", stat.name.c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.2f ms", stat.currentMicroseconds / 1000.0);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%.2f ms", stat.averageMicroseconds / 1000.0);
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%.2f/%.2f", stat.minMicroseconds / 1000.0, stat.maxMicroseconds / 1000.0);
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%.1f%%", stat.percentage);
                }
                
                ImGui::EndTable();
            }
        }
        ImGui::End();
        #endif
    }

private:
    Profiler() 
        : m_enabled(false)
        , m_frameAveraging(60)
        , m_frameTotalMicroseconds(0)
    {}
    
    ~Profiler() = default;
    
    // Disable copying
    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;
    
    struct RunningData
    {
        long long current = 0;
        long long average = 0;
        long long min = LLONG_MAX;
        long long max = 0;
        int callCount = 0;
        int frameCount = 0;
    };
    
    void UpdateAverages()
    {
        for (const auto& [name, time] : m_frameSections)
        {
            auto& data = m_runningAverages[name];
            
            data.current = time;
            data.callCount++;
            data.frameCount++;
            
            // Accumulate for averaging
            data.average = (data.average * (data.frameCount - 1) + time) / data.frameCount;
            
            if (time < data.min) data.min = time;
            if (time > data.max) data.max = time;
            
            // Reset frame count periodically to keep averages relevant
            if (data.frameCount >= m_frameAveraging)
            {
                data.frameCount = 0;
            }
        }
    }
    
    bool m_enabled;
    int m_frameAveraging;
    
    std::chrono::time_point<std::chrono::high_resolution_clock> m_frameStart;
    long long m_frameTotalMicroseconds;
    
    std::unordered_map<std::string, long long> m_frameSections;
    std::unordered_map<std::string, RunningData> m_runningAverages;
};

/**
 * Example usage in code:
 * 
 * // In your main game loop
 * void GameLoop::Update(float deltaTime)
 * {
 *     Profiler::Get().SetEnabled(true);  // Enable in debug builds
 *     Profiler::Get().BeginFrame();
 *     
 *     {
 *         PROFILE_SCOPE("Input Processing");
 *         m_inputManager->Update();
 *     }
 *     
 *     {
 *         PROFILE_SCOPE("Physics Update");
 *         m_physicsSystem->Update(deltaTime);
 *     }
 *     
 *     {
 *         PROFILE_SCOPE("Render");
 *         m_renderer->Render();
 *     }
 *     
 *     Profiler::Get().EndFrame();
 *     
 *     // Show profiler window
 *     Profiler::Get().RenderUI();
 * }
 */

} // namespace NK