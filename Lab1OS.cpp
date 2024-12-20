#include <iostream>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <memory>
#include <queue> 
using namespace std;

// Данные события
class EventData {
public:
    int id;
    explicit EventData(int id) : id(id) {}
};

// Монитор
class Monitor {
public:
    mutex mtx;                    
    condition_variable cond;     
    bool stopRequest = false;  
    queue<shared_ptr<EventData>> eventQueue; 

public:

    // Метод для проверки, есть ли события в очереди
    bool hasEvents() {
        return !eventQueue.empty();
    }

    // Метод для события (поставщик)
    void produceEvent(int id) {
        lock_guard<mutex> lock(mtx);
        auto event = make_shared<EventData>(id);
        eventQueue.push(event);
        cout << "Producer: Event " << id << " produced." << endl;
        cond.notify_one();
    }

    // Метод для обработки события (потребитель)
    void consumeEvent() {
        unique_lock<mutex> lock(mtx); 
        cond.wait(lock, [this]() { return hasEvents() || stopRequest; });

        if (stopRequest && eventQueue.empty()) {
            cout << "Consumer: Stopping, no more events." << endl;
            return;
        }

        auto event = eventQueue.front();
        eventQueue.pop();

        cout << "Consumer: Event " << event->id << " consumed." << endl;
    }

    // Метод для остановки потока
    void stop() {
        lock_guard<mutex> lock(mtx);
        stopRequest = true;
        cond.notify_all();
    }
};

// Поток-поставщик
void producer(Monitor& monitor, int maxEvents) {
    int eventCounter = 0;
    while (eventCounter < maxEvents) {
        this_thread::sleep_for(chrono::seconds(1));
        monitor.produceEvent(eventCounter++);
    }
    monitor.stop();
}

// Поток-потребитель
void consumer(Monitor& monitor, int maxEvents) {
    while (true) {
        monitor.consumeEvent();
        if (monitor.stopRequest && monitor.eventQueue.empty()) {
            break;
        }
    }
}

int main() {
    Monitor monitor;

    const int maxEvents = 5;

    thread producerThread(producer, ref(monitor), maxEvents);  // Поток-поставщик
    thread consumerThread(consumer, ref(monitor), maxEvents);  // Поток-потребитель

    producerThread.join();  // Ожидаем завершения потока-поставщика
    consumerThread.join();  // Ожидаем завершения потока-потребителя

    return 0;
}