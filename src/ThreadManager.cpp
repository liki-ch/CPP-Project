#include "../include/ThreadManager.h"
#include <stdexcept>

ThreadManager::ThreadManager(size_t numThreads) 
    : numThreads(numThreads), running(false) {
    if (numThreads <= 0) {
        throw std::invalid_argument("Number of threads must be positive");
    }
}

ThreadManager::~ThreadManager() {
    stop();
}

void ThreadManager::start() {
    if (running) return;
    running = true;
    activeThreads = 0;
    threadLoads.resize(numThreads, 0);
    
    for (size_t i = 0; i < numThreads; ++i) {
        threads.push_back(std::thread(&ThreadManager::workerThread, this, i));
    }
}

void ThreadManager::stop() {
    {
        std::lock_guard<std::mutex> lock(taskMutex);
        running = false;
    }
    taskCondition.notify_all();
    
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    threads.clear();
}

void ThreadManager::addTask(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(taskMutex);
        taskQueue.push(task);
    }
    taskCondition.notify_one();
}

bool ThreadManager::isRunning() const {
    return running;
}

size_t ThreadManager::getNumThreads() const {
    return numThreads;
}

void ThreadManager::setNumThreads(size_t newNumThreads) {
    if (newNumThreads == numThreads) return;
    
    stop();
    numThreads = newNumThreads;
    threadLoads.resize(numThreads, 0);
    if (running) {
        start();
    }
}

size_t ThreadManager::getTaskCount() const {
    std::lock_guard<std::mutex> lock(taskMutex);
    return taskQueue.size();
}

void ThreadManager::waitForCompletion() {
    std::unique_lock<std::mutex> lock(completionMutex);
    taskCondition.wait(lock, [this]() {
        return (taskQueue.empty() && activeThreads == 0);
    });
}

size_t ThreadManager::getActiveThreadCount() const {
    return activeThreads;
}

void ThreadManager::processNextTask() {
    std::function<void()> task;
    {
        std::lock_guard<std::mutex> lock(taskMutex);
        if (taskQueue.empty()) return;
        
        task = taskQueue.front();
        taskQueue.pop();
    }
    
    if (task) {
        task();
    }
}

void ThreadManager::workerThread(size_t threadId) {
    while (running) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(taskMutex);
            taskCondition.wait(lock, [this]() {
                return !running || !taskQueue.empty();
            });
            
            if (!running && taskQueue.empty()) {
                return;
            }
            
            if (!taskQueue.empty()) {
                task = taskQueue.front();
                taskQueue.pop();
                activeThreads++;
                threadLoads[threadId]++;
            }
        }
        
        if (task) {
            task();
            
            {
                std::lock_guard<std::mutex> lock(completionMutex);
                activeThreads--;
            }
            taskCondition.notify_all();
        }
    }
}