#pragma once

#include <Events/EventBus.h>
#include <Core-ECS/Entity.h>

namespace NK
{

// ============================================================================
// PLAYER EVENTS
// ============================================================================

/**
 * PlayerDiedEvent - Fired when a player dies
 */
struct PlayerDiedEvent : public Event
{
    Entity player;
    int finalScore;
    std::string cause;
    
    PlayerDiedEvent(Entity p, int score, const std::string& c = "unknown")
        : player(p), finalScore(score), cause(c) {}
    
    const char* GetEventName() const override { return "PlayerDiedEvent"; }
};

/**
 * PlayerSpawnedEvent - Fired when a player spawns
 */
struct PlayerSpawnedEvent : public Event
{
    Entity player;
    glm::vec3 spawnPosition;
    
    PlayerSpawnedEvent(Entity p, const glm::vec3& pos)
        : player(p), spawnPosition(pos) {}
    
    const char* GetEventName() const override { return "PlayerSpawnedEvent"; }
};

/**
 * ScoreChangedEvent - Fired when player score changes
 */
struct ScoreChangedEvent : public Event
{
    Entity player;
    int oldScore;
    int newScore;
    
    ScoreChangedEvent(Entity p, int oldS, int newS)
        : player(p), oldScore(oldS), newScore(newS) {}
    
    const char* GetEventName() const override { return "ScoreChangedEvent"; }
};

// ============================================================================
// GAME STATE EVENTS
// ============================================================================

/**
 * GameStateChangedEvent - Fired when game state changes
 */
enum class GameState
{
    Menu,
    Playing,
    Paused,
    GameOver
};

struct GameStateChangedEvent : public Event
{
    GameState oldState;
    GameState newState;
    
    GameStateChangedEvent(GameState oldS, GameState newS)
        : oldState(oldS), newState(newS) {}
    
    const char* GetEventName() const override { return "GameStateChangedEvent"; }
};

/**
 * LevelLoadedEvent - Fired when a level finishes loading
 */
struct LevelLoadedEvent : public Event
{
    std::string levelName;
    
    explicit LevelLoadedEvent(const std::string& name) : levelName(name) {}
    
    const char* GetEventName() const override { return "LevelLoadedEvent"; }
};

// ============================================================================
// INPUT EVENTS
// ============================================================================

/**
 * KeyPressedEvent - Fired when a key is pressed
 */
struct KeyPressedEvent : public Event
{
    int keyCode;
    bool isRepeat;
    
    KeyPressedEvent(int key, bool repeat = false)
        : keyCode(key), isRepeat(repeat) {}
    
    const char* GetEventName() const override { return "KeyPressedEvent"; }
};

/**
 * MouseMovedEvent - Fired when mouse moves
 */
struct MouseMovedEvent : public Event
{
    float x, y;       // Current position
    float dx, dy;     // Delta from last frame
    
    MouseMovedEvent(float mouseX, float mouseY, float deltaX, float deltaY)
        : x(mouseX), y(mouseY), dx(deltaX), dy(deltaY) {}
    
    const char* GetEventName() const override { return "MouseMovedEvent"; }
};

// ============================================================================
// PHYSICS EVENTS
// ============================================================================

/**
 * CollisionStartedEvent - Fired when two bodies start colliding
 */
struct CollisionStartedEvent : public Event
{
    Entity entityA;
    Entity entityB;
    glm::vec3 contactPoint;
    
    CollisionStartedEvent(Entity a, Entity b, const glm::vec3& point)
        : entityA(a), entityB(b), contactPoint(point) {}
    
    const char* GetEventName() const override { return "CollisionStartedEvent"; }
};

/**
 * TriggerEnteredEvent - Fired when entering a trigger volume
 */
struct TriggerEnteredEvent : public Event
{
    Entity trigger;
    Entity other;
    
    TriggerEnteredEvent(Entity t, Entity o)
        : trigger(t), other(o) {}
    
    const char* GetEventName() const override { return "TriggerEnteredEvent"; }
};

// ============================================================================
// UI EVENTS
// ============================================================================

/**
 * ButtonClickedEvent - Fired when a UI button is clicked
 */
struct ButtonClickedEvent : public Event
{
    std::string buttonId;
    
    explicit ButtonClickedEvent(const std::string& id) : buttonId(id) {}
    
    const char* GetEventName() const override { return "ButtonClickedEvent"; }
};

// ============================================================================
// NETWORK EVENTS
// ============================================================================

/**
 * PlayerConnectedEvent - Fired when a player connects (multiplayer)
 */
struct PlayerConnectedEvent : public Event
{
    int playerId;
    std::string playerName;
    
    PlayerConnectedEvent(int id, const std::string& name)
        : playerId(id), playerName(name) {}
    
    const char* GetEventName() const override { return "PlayerConnectedEvent"; }
};

/**
 * PlayerDisconnectedEvent - Fired when a player disconnects
 */
struct PlayerDisconnectedEvent : public Event
{
    int playerId;
    std::string reason;
    
    PlayerDisconnectedEvent(int id, const std::string& r = "unknown")
        : playerId(id), reason(r) {}
    
    const char* GetEventName() const override { return "PlayerDisconnectedEvent"; }
};

// ============================================================================
// USAGE EXAMPLE
// ============================================================================

/*
// Example: Using events in a game system

class GameManager : public EventListener {
public:
    GameManager() : EventListener(&m_eventBus) {
        // Subscribe to events
        m_eventBus.Subscribe<PlayerDiedEvent>(this);
        m_eventBus.Subscribe<ScoreChangedEvent>(this);
    }
    
    void OnEvent(const Event& event) override {
        // Handle all events here, or use dynamic_cast
        if (auto* e = dynamic_cast<const PlayerDiedEvent*>(&event)) {
            HandlePlayerDeath(*e);
        }
        else if (auto* e = dynamic_cast<const ScoreChangedEvent*>(&event)) {
            HandleScoreChange(*e);
        }
    }
    
private:
    EventBus m_eventBus;
    
    void HandlePlayerDeath(const PlayerDiedEvent& e) {
        std::cout << "Player died! Score: " << e.finalScore << std::endl;
        m_eventBus.Publish<GameStateChangedEvent>(GameState::Playing, GameState::GameOver);
    }
    
    void HandleScoreChange(const ScoreChangedEvent& e) {
        std::cout << "Score: " << e.oldScore << " -> " << e.newScore << std::endl;
    }
};

// Triggering events
void OnEnemyKilled(Entity enemy) {
    m_eventBus.Publish<ScoreChangedEvent>(player, currentScore, currentScore + 100);
    m_eventBus.Publish<PlayerDiedEvent>(enemy, 0, "killed");
}
*/

} // namespace NK