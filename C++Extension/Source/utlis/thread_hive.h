/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef THREAD_HIVE_H
#define THREAD_HIVE_H

#include "fast_queue.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <stdexcept>

class ThreadHive {
private:

    // Type-defines
    typedef void (*TaskCallback)(void* user_data, ThreadHive* hive);

    // Disable copy constructor and assignment operator
    ThreadHive(const ThreadHive& other);
    ThreadHive& operator=(const ThreadHive& other);

    // Structures
    struct Task {
        TaskCallback m_task_callback;
        void* m_user_data;
    };

    // Variables
    FastQueue<Task> m_tasks;
    std::thread** m_bees;
    std::mutex m_queue_mutex;
    std::condition_variable m_cv_task;
    std::condition_variable m_cv_finished;
    bool m_terminate;
    unsigned int m_num_bees;
    unsigned int m_busy;

    // Helper Functions
    static void thread_task(ThreadHive* hive);

public:
    ThreadHive(unsigned int num_threads);
    virtual ~ThreadHive();

    unsigned int get_num_tasks();
    void enqueue(TaskCallback task_callback, void* user_data);
    void wait_until_finished();
};

#endif  /* THREAD_HIVE_H */
