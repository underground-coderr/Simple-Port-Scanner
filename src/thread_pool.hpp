#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

using namespace std;

class ThreadPool {
public:

    // spin up `num_threads` workers immediately on construction
    ThreadPool(int num_threads) : stop(false) {
        for (int i = 0; i < num_threads; i++) {
            workers.emplace_back([this] {
                while (true) {
                    function<void()> task;

                    // grab the lock, wait until there's something to do
                    {
                        unique_lock<mutex> lock(queue_mutex);
                        condition.wait(lock, [this] {
                            return stop || !tasks.empty();
                        });

                        // if we're shutting down and nothing left, exit
                        if (stop && tasks.empty())
                            return;

                        // grab the next task and release the lock
                        task = move(tasks.front());
                        tasks.pop();
                    }

                    task(); // do the actual work outside the lock
                }
            });
        }
    }

    // push a new task onto the queue
    void enqueue(function<void()> task) {
        {
            unique_lock<mutex> lock(queue_mutex);
            tasks.push(move(task));
        }
        condition.notify_one(); // wake up one sleeping worker
    }

    // destructor — tell everyone to stop, then wait for them to finish
    ~ThreadPool() {
        {
            unique_lock<mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all(); // wake all workers so they can see stop=true
        for (auto& worker : workers)
            worker.join();
    }

private:
    vector<thread>          workers;
    queue<function<void()>> tasks;
    mutex                   queue_mutex;
    condition_variable      condition;
    bool                    stop;
};