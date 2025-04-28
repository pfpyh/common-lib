#pragma once

#include "CommonHeader.hpp"
#include "common/Factory.hpp"

#include <vector>
#include <memory>
#include <algorithm>

namespace common
{
template <typename... Args>
class BaseObserver
{
public :
    virtual ~BaseObserver() = default;
    virtual auto onEvent(Args... args) -> void = 0;
};

/**
 * @brief This class is a base class for an observer that can be notified of certain events.
 *
 * This class is a base class for an observer that can be notified of certain events.
 * It provides a way to create and manage observers, allowing for event driven programming.
 * The create function takes a variable number of arguments and returns a shared pointer to a derived class.
 * The onEvent function is a pure virtual function that must be implemented in a derived class.
 *
 * @tparam Derived The type of derived class.
 * @tparam Args The types of arguments to be passed to the onEvent function.
 */
template <typename Derived, typename... Args>
class Observer : public BaseObserver<Args...> 
               , public Factory<Derived>
{
    friend class Factory<Derived>;

private :
    /**
     * @brief Creates an object of the derived type using the provided arguments.
     *
     * This function takes a variable number of arguments and returns a shared pointer to a derived class.
     * The derived class must implement the pure virtual function onEvent.
     *
     * @param args The arguments to be passed to the onEvent function.
     * @return A shared pointer to an object of the derived type.
     */
    template <typename... ConstructTypes>
    static auto __create(ConstructTypes... types) noexcept -> std::shared_ptr<Derived>
    {
        return std::shared_ptr<Derived>(new Derived(std::forward<ConstructTypes>(types)...));
    }

public :
    /**
     * @brief This function is a pure virtual function that must be implemented in a derived class.
     *
     * This function is called when an event occurs and must be implemented in a derived class.
     * The derived class must take the same number and type of arguments as the Args parameter.
     *
     * @param args The arguments to be passed to the onEvent function.
     */
    virtual auto onEvent(Args... args) -> void = 0;
};

/**
 * @brief Observer pattern subject class.
 *
 * This class is the subject of the observer pattern. It holds a list of observers and
 * provides methods to register and unregister observers. It also provides a method to
 * notify all registered observers of an event.
 *
 * @tparam Args The types of arguments to be passed to the onEvent function.
 */
template <typename... Args>
class Subject
{
private :
    /**
     * @brief The list of registered observers.
     *
     * This vector stores the registered observers. The observers are stored as shared
     * pointers to ensure that the observers are not deleted until all references to them
     * are destroyed.
     */
    std::vector<std::shared_ptr<BaseObserver<Args...>>> _observers;

public :
    /**
     * @brief Registers an observer to receive events.
     *
     * This method registers an observer to receive events. The observer is stored as a
     * shared pointer to ensure that the observer is not deleted until all references to
     * it are destroyed.
     *
     * @param observer The observer to be registered.
     */
    auto regist(std::shared_ptr<BaseObserver<Args...>> observer) noexcept -> void
    {
        _observers.emplace_back(std::move(observer));
    }

    /**
     * @brief Unregisters an observer from receiving events.
     *
     * This method unregisters an observer from receiving events. The observer is removed
     * from the list of registered observers.
     *
     * @param observer The observer to be unregistered.
     */
    auto unregist(BaseObserver<Args...>& observer) noexcept -> void
    {
        auto target = observer.get_ptr();
        _observers.erase(
            std::remove_if(_observers.begin(), _observers.end(),
                           [&target](const std::shared_ptr<BaseObserver<Args...>>& obs) {
                               return obs == target;
                           }),
            _observers.end());
    }

    /**
     * @brief Unregisters an observer from receiving events.
     *
     * This method unregisters an observer from receiving events. The observer is removed
     * from the list of registered observers.
     *
     * @param observer The observer to be unregistered.
     */
    auto unregist(std::shared_ptr<BaseObserver<Args...>> observer) noexcept -> void
    {
        _observers.erase(
            std::remove_if(_observers.begin(), _observers.end(),
                           [&observer](const std::shared_ptr<BaseObserver<Args...>>& obs) {
                               return obs == observer;
                           }), _observers.end());
    }

    /**
     * @brief Notifies all registered observers of an event.
     *
     * This method notifies all registered observers of an event by calling the onEvent
     * function of each observer with the provided arguments.
     *
     * @param args The arguments to be passed to the onEvent function of each observer.
     */
    auto notify(Args... args) -> void
    {
        for(auto observer : _observers) { observer->onEvent(args...); }
    }
};
};