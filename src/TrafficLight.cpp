#include "TrafficLight.h"

using namespace std::chrono;
using namespace std::chrono_literals;

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function.
    std::unique_lock<std::mutex> queue_lck(_queue_mtx);
    _queue_cv.wait(queue_lck, [this] { return !_queue.empty(); });
    T message = std::move(_queue.front());
    _queue.pop_front();

    return message;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::unique_lock<std::mutex> queue_lck(_queue_mtx);
    _queue.push_back(std::move(msg));
    _queue_cv.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    // the thread should not be running, no synchronization needed
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // return if already green
    if (getCurrentPhase() == TrafficLightPhase::green) {
        return;
    }
    
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while (_messages.receive() != TrafficLightPhase::green) {
        std::this_thread::yield();
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    std::lock_guard<std::mutex> lock(_currentPhase_mtx); // read is synchronized
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
   threads.emplace_back(&TrafficLight::cycleThroughPhases, this);
}

void TrafficLight::toggleCurrentPhase() {
    std::lock_guard<std::mutex> lock(_currentPhase_mtx); // write is synchronized
    _currentPhase = _currentPhase == TrafficLightPhase::red ? TrafficLightPhase::green : TrafficLightPhase::red;
    _messages.send(std::move(_currentPhase));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases() {
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.
    auto next(high_resolution_clock::now() + seconds(_dis(_gen)));
    
    for (time_point<high_resolution_clock> now = high_resolution_clock::now();;now = high_resolution_clock::now()) {
        if (now > next) {
            toggleCurrentPhase();
            next = now + seconds(_dis(_gen));
        }

        std::this_thread::sleep_for(milliseconds(1));
    }
}
