#pragma once

#include <typeindex>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>

namespace NK
{

/**
 * Event - Base class for all events
 * Derive from this to create custom event types
 */
struct Event
{
    virtual ~Event() = default;
    
    // Optional: Get event name for debugging
    virtual const char* GetEventName() const { return "Event"; }
};

/**
 * IEventListener - Interface for event listeners
 * Implement this to receive events of specific types
 */
class IEventListener
{
public:
    virtual ~IEventListener() = default;
    
    // Called when an event is dispatched
    virtual void OnEvent(const Event& event) = 0;
};

/**
 * EventBus - Central event dispatch system
 * 
 * Usage:
 *   EventBus eventBus;
 *   
 *   // Subscribe to events
 *   eventBus.Subscribe<PlayerDiedEvent>(this);
 *   
 *   // Dispatch events
 *   eventBus.Publish<PlayerDiedEvent>(player, score);
 *   
 *   // Unsubscribe
 *   eventBus.Unsubscribe<PlayerDiedEvent>(this);
 */
class EventBus
{
public:
    EventBus() = default;
    ~EventBus() = default;
    
    // Prevent copying
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    
    /**
     * Subscribe to an event type
     * T - Event type to subscribe to
     * L - Listener pointer
     */
    template<typename T>
    void Subscribe(IEventListener* listener)
    {
        static_assert(std::is_base_of<Event, T>::value, "T must derive from Event");
        
        auto type = std::type_index(typeid(T));
        m_listeners[type].push_back(listener);
        
        #ifndef NDEBUG
        std::cout << "EventBus: Subscribed to " << type.name() << std::endl;
        #endif
    }
    
    /**
     * Unsubscribe from an event type
     */
    template<typename T>
    void Unsubscribe(IEventListener* listener)
    {
        static_assert(std::is_base_of<Event, T>::value, "T must derive from Event");
        
        auto type = std::type_index(typeid(T));
        auto& listeners = m_listeners[type];
        
        listeners.erase(
            std::remove(listeners.begin(), listeners.end(), listener),
            listeners.end()
        );
    }
    
    /**
     * Publish an event (by constructing it in-place)
     * T - Event type
     * Args - Arguments to construct the event
     */
    template<typename T, typename... Args>
    void Publish(Args&&... args)
    {
        static_assert(std::is_base_of<Event, T>::value, "T must derive from Event");
        
        T event(std::forward<Args>(args)...);
        Dispatch<T>(event);
    }
    
    /**
     * Publish an existing event (by reference)
     */
    template<typename T>
    void Publish(const T& event)
    {
        static_assert(std::is_base_of<Event, T>::value, "T must derive from Event");
        Dispatch<T>(event);
    }
    
    /**
     * Clear all subscriptions for a listener
     */
    void UnsubscribeAll(IEventListener* listener)
    {
        for (auto& [type, listeners] : m_listeners)
        {
            listeners.erase(
                std::remove(listeners.begin(), listeners.end(), listener),
                listeners.end()
            );
        }
    }
    
    /**
     * Clear all event subscriptions
     */
    void Clear()
    {
        m_listeners.clear();
    }

private:
    template<typename T>
    void Dispatch(const T& event)
    {
        auto type = std::type_index(typeid(T));
        auto it = m_listeners.find(type);
        
        if (it != m_listeners.end())
        {
            for (auto* listener : it->second)
            {
                if (listener)
                {
                    listener->OnEvent(event);
                }
            }
        }
    }
    
    // Event type to listener list mapping
    std::unordered_map<std::type_index, std::vector<IEventListener*>> m_listeners;
};

/**
 * EventListener - Base class for convenience
 * Provides default implementations
 */
class EventListener : public IEventListener
{
public:
    explicit EventListener(EventBus* bus) : m_eventBus(bus) {}
    
    ~EventListener() override
    {
        if (m_eventBus)
        {
            m_eventBus->UnsubscribeAll(this);
        }
    }
    
    // Default implementation does nothing
    // Override in derived classes to handle specific events
    
protected:
    EventBus* m_eventBus = nullptr;
};

} // namespace NK