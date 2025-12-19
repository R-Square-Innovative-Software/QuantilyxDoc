/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "ThreadPool.h"
#include "Logger.h"
#include <QThreadPool>
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>
#include <QTime>
#include <QCoreApplication>
#include <QThread>

namespace QuantilyxDoc {

class Task::Private {
public:
    Private(std::function<void()> runnable_func, const QString& name_val, Priority priority_val)
        : runnable(std::move(runnable_func)), name(name_val), priority(priority_val),
          state(State::Queued), canceled(false), autoDelete(true) {}
    std::function<void()> runnable;
    QString name;
    Priority priority;
    State state;
    QDateTime enqueueTime;
    QDateTime startTime;
    QDateTime finishTime;
    mutable QMutex stateMutex; // Protect state changes
    bool canceled;
    QVariant userData;
    bool autoDelete;
};

Task::Task(std::function<void()> runnable, const QString& name, Priority priority)
    : d(new Private(std::move(runnable), name, priority))
{
    d->enqueueTime = QDateTime::currentDateTime();
}

void Task::run()
{
    QMutexLocker locker(&d->stateMutex);
    if (d->canceled) {
        d->state = State::Canceled;
        LOG_DEBUG("Task " << (d->name.isEmpty() ? QString::number(reinterpret_cast<quintptr>(this)) : d->name) << " was canceled before execution.");
        return;
    }
    d->state = State::Running;
    d->startTime = QDateTime::currentDateTime();
    locker.unlock(); // Unlock while running the task function

    try {
        if (d->runnable) {
            d->runnable();
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Task " << (d->name.isEmpty() ? QString::number(reinterpret_cast<quintptr>(this)) : d->name) << " threw exception: " << e.what());
    } catch (...) {
        LOG_ERROR("Task " << (d->name.isEmpty() ? QString::number(reinterpret_cast<quintptr>(this)) : d->name) << " threw unknown exception.");
    }

    locker.relock(); // Re-lock to update final state
    d->state = State::Finished;
    d->finishTime = QDateTime::currentDateTime();
    LOG_DEBUG("Task " << (d->name.isEmpty() ? QString::number(reinterpret_cast<quintptr>(this)) : d->name) << " finished execution.");
}

quintptr Task::id() const
{
    return reinterpret_cast<quintptr>(this);
}

QString Task::name() const
{
    QMutexLocker locker(&d->stateMutex);
    return d->name;
}

Task::Priority Task::priority() const
{
    QMutexLocker locker(&d->stateMutex);
    return d->priority;
}

Task::State Task::state() const
{
    QMutexLocker locker(&d->stateMutex);
    return d->state;
}

QDateTime Task::enqueueTime() const
{
    QMutexLocker locker(&d->stateMutex);
    return d->enqueueTime;
}

QDateTime Task::startTime() const
{
    QMutexLocker locker(&d->stateMutex);
    return d->startTime;
}

QDateTime Task::finishTime() const
{
    QMutexLocker locker(&d->stateMutex);
    return d->finishTime;
}

QTime Task::executionTime() const
{
    QMutexLocker locker(&d->stateMutex);
    if (d->startTime.isValid() && d->finishTime.isValid()) {
        qint64 ms = d->startTime.msecsTo(d->finishTime);
        return QTime(0, 0).addMSecs(ms);
    }
    return QTime(); // Invalid time if not finished
}

bool Task::cancel()
{
    QMutexLocker locker(&d->stateMutex);
    if (d->state == State::Queued) {
        d->canceled = true;
        d->state = State::Canceled;
        LOG_DEBUG("Task " << (d->name.isEmpty() ? QString::number(reinterpret_cast<quintptr>(this)) : d->name) << " was canceled.");
        return true;
    }
    return false; // Cannot cancel if already running or finished
}

bool Task::wasCanceled() const
{
    QMutexLocker locker(&d->stateMutex);
    return d->canceled;
}

void Task::setUserData(const QVariant& data)
{
    QMutexLocker locker(&d->stateMutex);
    d->userData = data;
}

QVariant Task::userData() const
{
    QMutexLocker locker(&d->stateMutex);
    return d->userData;
}

void Task::setAutoDelete(bool autoDel)
{
    QMutexLocker locker(&d->stateMutex);
    d->autoDelete = autoDel;
}

// --- ThreadPool Implementation ---

class ThreadPool::Private {
public:
    Private(ThreadPool* q_ptr) : q(q_ptr), qtThreadPool(nullptr), maxCount(0), totalSubmitted(0), totalCompleted(0) {}

    ThreadPool* q;
    QScopedPointer<QThreadPool> qtThreadPool;
    mutable QMutex mutex; // Protect access to task lists/maps
    QHash<quintptr, Task*> allTasks; // All tasks (queued, running, finished)
    QHash<Task::State, QSet<Task*>> tasksByState; // Group tasks by state
    int maxCount; // Cached max thread count
    quint64 totalSubmitted;
    quint64 totalCompleted;
    QWaitCondition condition; // For waitForDone

    // Helper to get or create the QThreadPool instance
    QThreadPool* getOrCreateQtThreadPool() {
        if (!qtThreadPool) {
            qtThreadPool.reset(QThreadPool::globalInstance()); // Use global instance by default
            // Or create a new one: qtThreadPool.reset(new QThreadPool());
        }
        return qtThreadPool.data();
    }

    // Helper to update task state and internal tracking
    void updateTaskState(Task* task, Task::State oldState, Task::State newState) {
        QMutexLocker locker(&mutex);
        // Remove from old state group
        if (tasksByState.contains(oldState)) {
            tasksByState[oldState].remove(task);
        }
        // Add to new state group
        tasksByState[newState].insert(task);

        // Update counters
        if (newState == Task::State::Finished || newState == Task::State::Canceled) {
            totalCompleted++;
        }

        // Wake up waitForDone if necessary
        if (newState == Task::State::Finished || newState == Task::State::Canceled) {
            condition.wakeAll();
        }

        emit q->taskStateChanged(task, newState);
        switch (newState) {
            case Task::State::Queued:
                emit q->taskQueued(task);
                break;
            case Task::State::Running:
                emit q->taskStarted(task);
                break;
            case Task::State::Finished:
            case Task::State::Canceled:
                emit q->taskFinished(task);
                break;
            default:
                break;
        }
    }
};

// Static instance pointer
ThreadPool* ThreadPool::s_instance = nullptr;

ThreadPool& ThreadPool::instance()
{
    if (!s_instance) {
        s_instance = new ThreadPool();
    }
    return *s_instance;
}

ThreadPool::ThreadPool(QObject* parent)
    : QObject(parent)
    , d(new Private(this))
{
    // Initialize Qt's global thread pool settings if needed
    QThreadPool* pool = d->getOrCreateQtThreadPool();
    d->maxCount = pool->maxThreadCount(); // Cache initial value
    LOG_INFO("ThreadPool initialized with max threads: " << d->maxCount);
}

ThreadPool::~ThreadPool()
{
    // Ensure all tasks are finished before destruction
    waitForDone();
    // QThreadPool will handle cleanup of running tasks gracefully on application exit
}

void ThreadPool::submitTask(Task* task)
{
    if (!task) return;

    {
        QMutexLocker locker(&d->mutex);
        d->allTasks.insert(task->id(), task);
        d->tasksByState[Task::State::Queued].insert(task);
        d->totalSubmitted++;
    }
    emit queueStatusChanged(queuedTaskCount(), runningTaskCount(), activeThreadCount());

    // Submit to Qt's underlying pool
    // Qt's QThreadPool doesn't directly support custom priorities for individual tasks.
    // The priority of the QThread running the task is set globally or per thread.
    // We'll submit it normally. Priority could be handled by a custom queue implementation.
    QThreadPool* pool = d->getOrCreateQtThreadPool();
    pool->start(task, static_cast<int>(task->priority())); // QThreadPool accepts int priority

    LOG_DEBUG("Submitted task: " << task->name() << " (ID: " << task->id() << ")");
}

Task* ThreadPool::submitTask(std::function<void()> func, const QString& name, Task::Priority priority)
{
    Task* task = new Task(std::move(func), name, priority);
    submitTask(task); // Takes ownership
    return task;
}

bool ThreadPool::cancelTask(Task* task)
{
    if (!task) return false;
    return task->cancel(); // Let the Task object handle its own state change
    // The Task::run() method checks the canceled flag.
    // If it's already running, it cannot be canceled by this mechanism.
}

bool ThreadPool::cancelTaskById(quintptr taskId)
{
    Task* task = taskById(taskId);
    return cancelTask(task);
}

int ThreadPool::cancelAllQueuedTasks()
{
    QMutexLocker locker(&d->mutex);
    int canceledCount = 0;
    QSet<Task*> queuedTasks = d->tasksByState.value(Task::State::Queued, QSet<Task*>());
    for (Task* task : queuedTasks) {
        if (task->cancel()) { // This updates the task's internal state
             // The state change signal will update our internal lists via updateTaskState
             // which is ideally called from a watcher on the task's state or after run().
             // Since we can't easily hook into QRunnable's lifecycle that way,
             // we manually update our lists here for canceled tasks.
             d->tasksByState[Task::State::Queued].remove(task);
             d->tasksByState[Task::State::Canceled].insert(task);
             d->totalCompleted++;
             canceledCount++;
             emit taskStateChanged(task, Task::State::Canceled);
             emit taskFinished(task);
        }
    }
    emit queueStatusChanged(queuedTaskCount(), runningTaskCount(), activeThreadCount());
    LOG_DEBUG("Canceled " << canceledCount << " queued tasks.");
    return canceledCount;
}

int ThreadPool::maxThreadCount() const
{
    QMutexLocker locker(&d->mutex);
    QThreadPool* pool = d->getOrCreateQtThreadPool();
    d->maxCount = pool->maxThreadCount(); // Update cache
    return d->maxCount;
}

void ThreadPool::setMaxThreadCount(int count)
{
    QMutexLocker locker(&d->mutex);
    QThreadPool* pool = d->getOrCreateQtThreadPool();
    pool->setMaxThreadCount(count);
    d->maxCount = count; // Update cache
    LOG_INFO("ThreadPool max thread count set to: " << count);
}

int ThreadPool::activeThreadCount() const
{
    QMutexLocker locker(&d->mutex);
    QThreadPool* pool = d->getOrCreateQtThreadPool();
    return pool->activeThreadCount();
}

int ThreadPool::runningTaskCount() const
{
    QMutexLocker locker(&d->mutex);
    return d->tasksByState.value(Task::State::Running, QSet<Task*>()).size();
}

int ThreadPool::queuedTaskCount() const
{
    QMutexLocker locker(&d->mutex);
    return d->tasksByState.value(Task::State::Queued, QSet<Task*>()).size();
}

quint64 ThreadPool::totalTasksSubmitted() const
{
    QMutexLocker locker(&d->mutex);
    return d->totalSubmitted;
}

quint64 ThreadPool::totalTasksCompleted() const
{
    QMutexLocker locker(&d->mutex);
    return d->totalCompleted;
}

void ThreadPool::waitForDone(int msecs)
{
    QMutexLocker locker(&d->mutex);
    // Wait until the 'finished' or 'canceled' task set is equal to the total submitted
    // Or use QWaitCondition if we signal completion properly from Task::run.
    // A simpler way with QThreadPool is to call its waitForDone.
    QThreadPool* pool = d->getOrCreateQtThreadPool();
    // pool->waitForDone(msecs); // This waits for Qt's internal queue to drain, not our state tracking
    // Use condition variable based on our state tracking
    while (d->totalCompleted < d->totalSubmitted && (msecs == UINT_MAX || msecs > 0)) {
        QTime timer;
        timer.start();
        d->condition.wait(&d->mutex, 100); // Wait 100ms or until signaled
        if (msecs != UINT_MAX) {
            msecs -= timer.elapsed();
        }
    }
    if (msecs <= 0) {
        LOG_WARN("waitForDone: Timeout reached.");
    }
}

void ThreadPool::clearCompletedTasks()
{
    QMutexLocker locker(&d->mutex);
    QSet<Task*> completedTasks;
    completedTasks.unite(d->tasksByState.value(Task::State::Finished, QSet<Task*>()));
    completedTasks.unite(d->tasksByState.value(Task::State::Canceled, QSet<Task*>()));

    for (Task* task : completedTasks) {
        d->allTasks.remove(task->id());
        // State sets are already updated by updateTaskState or manual update in cancel
    }
    LOG_DEBUG("Cleared " << completedTasks.size() << " completed tasks from tracking.");
}

QList<Task*> ThreadPool::allTasks() const
{
    QMutexLocker locker(&d->mutex);
    return d->allTasks.values();
}

QList<Task*> ThreadPool::tasksByState(Task::State state) const
{
    QMutexLocker locker(&d->mutex);
    return d->tasksByState.value(state, QSet<Task*>()).values();
}

Task* ThreadPool::taskById(quintptr id) const
{
    QMutexLocker locker(&d->mutex);
    return d->allTasks.value(id, nullptr);
}

} // namespace QuantilyxDoc