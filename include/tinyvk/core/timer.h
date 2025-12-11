#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <mutex>
#include <algorithm>
#include <numeric>

namespace tinyvk {

/**
 * @brief High-resolution timer for measuring elapsed time
 */
class Timer {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::duration<double>;

    Timer() : m_start(Clock::now()), m_paused(false), m_pausedDuration(0) {}

    /// Reset the timer
    void reset() {
        m_start = Clock::now();
        m_pausedDuration = Duration(0);
        m_paused = false;
    }

    /// Pause the timer
    void pause() {
        if (!m_paused) {
            m_pauseStart = Clock::now();
            m_paused = true;
        }
    }

    /// Resume the timer
    void resume() {
        if (m_paused) {
            m_pausedDuration += Clock::now() - m_pauseStart;
            m_paused = false;
        }
    }

    /// Check if timer is paused
    [[nodiscard]] bool isPaused() const { return m_paused; }

    /// Get elapsed time in seconds
    [[nodiscard]] double elapsedSeconds() const {
        auto end = m_paused ? m_pauseStart : Clock::now();
        return Duration(end - m_start - m_pausedDuration).count();
    }

    /// Get elapsed time in milliseconds
    [[nodiscard]] double elapsedMilliseconds() const {
        return elapsedSeconds() * 1000.0;
    }

    /// Get elapsed time in microseconds
    [[nodiscard]] double elapsedMicroseconds() const {
        return elapsedSeconds() * 1000000.0;
    }

private:
    TimePoint m_start;
    TimePoint m_pauseStart;
    Duration m_pausedDuration;
    bool m_paused;
};

/**
 * @brief RAII timer that calls a callback when destroyed with elapsed time
 */
class ScopedTimer {
public:
    using Callback = std::function<void(double)>;

    /// Create a scoped timer that logs elapsed time on destruction
    explicit ScopedTimer(const std::string& name)
        : m_name(name), m_callback([name](double ms) {
            TVK_LOG_INFO("Timer '{}': {:.3f} ms", name, ms);
        }) {}

    /// Create a scoped timer with custom callback (receives elapsed ms)
    ScopedTimer(const std::string& name, Callback callback)
        : m_name(name), m_callback(std::move(callback)) {}

    ~ScopedTimer() {
        if (m_callback) {
            m_callback(m_timer.elapsedMilliseconds());
        }
    }

    // Non-copyable
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

    /// Get current elapsed time in milliseconds
    [[nodiscard]] double elapsedMilliseconds() const {
        return m_timer.elapsedMilliseconds();
    }

private:
    std::string m_name;
    Timer m_timer;
    Callback m_callback;
};

/**
 * @brief Statistics for a profiled section
 */
struct ProfileStats {
    std::string name;
    double lastTime = 0.0;      // Last recorded time in ms
    double totalTime = 0.0;     // Total accumulated time in ms
    double minTime = 0.0;       // Minimum time in ms
    double maxTime = 0.0;       // Maximum time in ms
    double avgTime = 0.0;       // Average time in ms
    uint64_t callCount = 0;     // Number of times called
    std::vector<double> history; // Recent history for graphing

    static constexpr size_t MaxHistorySize = 120;

    void addSample(double timeMs) {
        lastTime = timeMs;
        totalTime += timeMs;
        callCount++;
        
        if (callCount == 1) {
            minTime = maxTime = avgTime = timeMs;
        } else {
            minTime = std::min(minTime, timeMs);
            maxTime = std::max(maxTime, timeMs);
            avgTime = totalTime / static_cast<double>(callCount);
        }

        history.push_back(timeMs);
        if (history.size() > MaxHistorySize) {
            history.erase(history.begin());
        }
    }

    void reset() {
        lastTime = totalTime = minTime = maxTime = avgTime = 0.0;
        callCount = 0;
        history.clear();
    }
};

/**
 * @brief Performance profiler for tracking multiple named sections
 */
class Profiler {
public:
    static Profiler& instance() {
        static Profiler profiler;
        return profiler;
    }

    /// Start timing a section
    void begin(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_activeTimers[name] = Timer::Clock::now();
    }

    /// End timing a section
    void end(const std::string& name) {
        auto endTime = Timer::Clock::now();
        
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_activeTimers.find(name);
        if (it != m_activeTimers.end()) {
            auto duration = std::chrono::duration<double, std::milli>(endTime - it->second);
            
            auto& stats = m_stats[name];
            stats.name = name;
            stats.addSample(duration.count());
            
            m_activeTimers.erase(it);
        }
    }

    /// Get stats for a section
    [[nodiscard]] const ProfileStats* getStats(const std::string& name) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_stats.find(name);
        return it != m_stats.end() ? &it->second : nullptr;
    }

    /// Get all stats
    [[nodiscard]] std::vector<ProfileStats> getAllStats() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<ProfileStats> result;
        result.reserve(m_stats.size());
        for (const auto& [name, stats] : m_stats) {
            result.push_back(stats);
        }
        return result;
    }

    /// Reset all stats
    void reset() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stats.clear();
        m_activeTimers.clear();
    }

    /// Reset stats for a specific section
    void reset(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_stats.find(name);
        if (it != m_stats.end()) {
            it->second.reset();
        }
    }

    /// Enable/disable profiling
    void setEnabled(bool enabled) { m_enabled = enabled; }
    [[nodiscard]] bool isEnabled() const { return m_enabled; }

private:
    Profiler() = default;
    
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, Timer::TimePoint> m_activeTimers;
    std::unordered_map<std::string, ProfileStats> m_stats;
    bool m_enabled = true;
};

/**
 * @brief RAII profiler scope that automatically begins/ends a profile section
 */
class ProfileScope {
public:
    explicit ProfileScope(const std::string& name) : m_name(name) {
        if (Profiler::instance().isEnabled()) {
            Profiler::instance().begin(name);
        }
    }

    ~ProfileScope() {
        if (Profiler::instance().isEnabled()) {
            Profiler::instance().end(m_name);
        }
    }

    // Non-copyable
    ProfileScope(const ProfileScope&) = delete;
    ProfileScope& operator=(const ProfileScope&) = delete;

private:
    std::string m_name;
};

/**
 * @brief Frame time tracker for FPS and frame timing statistics
 */
class FrameTimer {
public:
    FrameTimer() = default;

    /// Call at the start of each frame
    void beginFrame() {
        m_frameStart = Timer::Clock::now();
    }

    /// Call at the end of each frame
    void endFrame() {
        auto now = Timer::Clock::now();
        auto duration = std::chrono::duration<double>(now - m_frameStart);
        m_deltaTime = duration.count();
        
        m_frameCount++;
        m_fpsTimer += m_deltaTime;
        
        if (m_fpsTimer >= 1.0) {
            m_fps = m_frameCount / m_fpsTimer;
            m_frameCount = 0;
            m_fpsTimer = 0.0;
        }

        // Track frame time history
        m_frameTimeHistory.push_back(static_cast<float>(m_deltaTime * 1000.0));
        if (m_frameTimeHistory.size() > MaxHistorySize) {
            m_frameTimeHistory.erase(m_frameTimeHistory.begin());
        }
    }

    /// Get delta time in seconds
    [[nodiscard]] double getDeltaTime() const { return m_deltaTime; }

    /// Get delta time in milliseconds  
    [[nodiscard]] double getDeltaTimeMs() const { return m_deltaTime * 1000.0; }

    /// Get current FPS (updates once per second)
    [[nodiscard]] double getFPS() const { return m_fps; }

    /// Get frame time history for graphing (float for ImGui compatibility)
    [[nodiscard]] const std::vector<float>& getFrameTimeHistory() const { 
        return m_frameTimeHistory; 
    }

    /// Get average frame time from history
    [[nodiscard]] double getAverageFrameTimeMs() const {
        if (m_frameTimeHistory.empty()) return 0.0;
        return std::accumulate(m_frameTimeHistory.begin(), m_frameTimeHistory.end(), 0.0f) 
               / static_cast<double>(m_frameTimeHistory.size());
    }

    /// Get min/max frame time from history
    [[nodiscard]] std::pair<float, float> getMinMaxFrameTimeMs() const {
        if (m_frameTimeHistory.empty()) return {0.0f, 0.0f};
        auto [minIt, maxIt] = std::minmax_element(m_frameTimeHistory.begin(), m_frameTimeHistory.end());
        return {*minIt, *maxIt};
    }

private:
    static constexpr size_t MaxHistorySize = 120;

    Timer::TimePoint m_frameStart;
    double m_deltaTime = 0.0;
    double m_fps = 0.0;
    double m_fpsTimer = 0.0;
    uint64_t m_frameCount = 0;
    std::vector<float> m_frameTimeHistory;
};

} // namespace tinyvk

// Alias for tvk namespace
namespace tvk {
    using tinyvk::Timer;
    using tinyvk::ScopedTimer;
    using tinyvk::ProfileStats;
    using tinyvk::Profiler;
    using tinyvk::ProfileScope;
    using tinyvk::FrameTimer;
}

// Convenience macros
#define TVK_PROFILE_SCOPE(name) tinyvk::ProfileScope _profileScope##__LINE__(name)
#define TVK_PROFILE_FUNCTION() TVK_PROFILE_SCOPE(__FUNCTION__)
#define TVK_SCOPED_TIMER(name) tinyvk::ScopedTimer _scopedTimer##__LINE__(name)
