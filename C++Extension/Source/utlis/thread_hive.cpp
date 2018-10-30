/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "thread_hive.h"

ThreadHive::ThreadHive(unsigned int num_threads) :
    m_num_bees(num_threads),
    m_terminate(false),
    m_busy(0)
{
    m_bees = (std::thread**)malloc(sizeof(std::thread*) * m_num_bees);
    for (unsigned int i = 0; i < m_num_bees; ++i) {
        m_bees[i] = new std::thread(thread_task, this);
    }
}

ThreadHive::~ThreadHive() {
    {
        std::unique_lock<std::mutex> guard(m_queue_mutex);
        m_terminate = true;
    }
    m_cv_task.notify_all();
    m_cv_finished.notify_all();
    for (unsigned int i = 0; i < m_num_bees; ++i)
        m_bees[i]->join();
    for (unsigned int i = 0; i < m_num_bees; ++i)
        delete m_bees[i];
    free(m_bees);
}

void ThreadHive::thread_task(ThreadHive* hive) {
    while (true) {
        std::unique_lock<std::mutex> guard(hive->m_queue_mutex);

        hive->m_cv_task.wait(guard, [hive] { return hive->m_terminate || !hive->m_tasks.empty(); });

        if (!hive->m_tasks.empty()) {
            Task job;

            ++hive->m_busy;

            hive->m_tasks.dequeue(job);

            guard.unlock();

            job.m_task_callback(job.m_user_data, hive);

            guard.lock();

            --hive->m_busy;

            hive->m_cv_finished.notify_one();
        }
        else if (hive->m_terminate)
            break;
    }
}

unsigned int ThreadHive::get_num_tasks() {
    std::unique_lock<std::mutex> guard(m_queue_mutex);
    return m_tasks.size();
}

void ThreadHive::enqueue(TaskCallback task_callback, void* user_data) {
    {
        std::unique_lock<std::mutex> guard(m_queue_mutex);
        if (m_terminate)
            throw std::runtime_error("Enqueue on stopped ThreadHive");
        else {
			Task task;
            task.m_task_callback = task_callback;
            task.m_user_data = user_data;
			m_tasks.enqueue(task);
        }
    }
    m_cv_task.notify_one();
}

void ThreadHive::wait_until_finished() {
    std::unique_lock<std::mutex> guard(m_queue_mutex);
    m_cv_finished.wait(guard, [this]() { return m_tasks.empty() && (m_busy == 0); });
}
