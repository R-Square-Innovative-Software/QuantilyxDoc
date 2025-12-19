/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_THREADPOOL_H
#define QUANTILYX_THREADPOOL_H

#include <QObject>
#include <QRunnable>
#include <QThreadPool>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QDateTime>
#include <memory>
#include <functional>

namespace QuantilyxDoc {

/**
 * @brief Represents a task that can be submitted to the thread pool.
 * Extends QRunnable to include metadata and status tracking.
 */
class Task : public QRunnable
{
public:
    enum class Priority {
        Low = QThread::LowPriority,
        Normal = QThread::NormalPriority,
        High = QThread::HighPriority,
        Critical = QThread::TimeCriticalPriority
    };

    enum class State {
        Queued,
        Running,
        Finished,
        Canceled
    };

    /**
     * @brief Constructor for a task.
     * @param runnable The function or lambda to execute.
     * @param name Optional name for the task.
     * @param priority Priority of the task.
     */
    explicit Task(std::function<void()> runnable, const QString& name = QString(), Priority priority = Priority::Normal);

    /**
     * @brief Run the task's function.
     * This is called by the thread pool.
     */
    void run() override;

    /**
     * @brief Get the task's unique ID.
     * @return Unique ID.
     */
    quintptr id() const;

    /**
     * @brief Get the task's name.
     * @return Name.
     */
    QString name() const;

    /**
     * @brief Get the task's priority.
     * @return Priority.
     */
    Priority priority() const;

    /**
     * @brief Get the task's current state.
     * @return State.
     */
    State state() const;

    /**
     * @brief Get the time the task was enqueued.
     * @return Enqueue time.
     */
    QDateTime enqueueTime() const;

    /**
     * @brief Get the time the task started running.
     * @return Start time, or invalid QDateTime if not started.
     */
    QDateTime startTime() const;

    /**
     * @brief Get the time the task finished running.
     * @return Finish time, or invalid QDateTime if not finished.
     */
    QDateTime finishTime() const;

    /**
     * @brief Get the execution time of the task.
     * @return Duration, or null duration if not finished.
     */
    QTime executionTime() const;

    /**
     * @brief Cancel the task if it's still queued.
     * @return True if cancellation was successful (i.e., task was queued).
     */
    bool cancel();

    /**
     * @brief Check if the task was canceled.
     * @return True if canceled.
     */
    bool wasCanceled() const;

    /**
     * @brief Set user-defined data associated with the task.
     * @param data Arbitrary data.
     */
    void setUserData(const QVariant& data);

    /**
     * @brief Get user-defined data associated with the task.
     * @return Stored data.
     */
    QVariant userData() const;

    // Required by QRunnable
    void setAutoDelete(bool autoDel) override;

private:
    quintptr m_id;
    QString m_name;
    Priority m_priority;
    State m_state;
    QDateTime m_enqueueTime;
    QDateTime m_startTime;
    QDateTime m_finishTime;
    std::function<void()> m_runnable;
    mutable QMutex m_stateMutex; // Protect state changes
    bool m_canceled;
    QVariant m_userData;
    bool m_autoDelete;
};

/**
 * @brief Enhanced thread pool manager.
 * 
 * Provides a higher-level interface around Qt's QThreadPool, adding
 * features like task tracking, priority queues, and performance monitoring.
 */
class ThreadPool : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     * @param parent Parent object.
     */
    explicit ThreadPool(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~ThreadPool() override;

    /**
     * @brief Get singleton instance.
     * @return Reference to the global ThreadPool instance.
     */
    static ThreadPool& instance();

    /**
     * @brief Submit a task to the pool.
     * @param task The task to submit. The pool takes ownership.
     */
    void submitTask(Task* task);

    /**
     * @brief Submit a simple function as a task.
     * @param func The function to run.
     * @param name Optional name for the task.
     * @param priority Priority of the task.
     * @return Pointer to the created Task object (for tracking/cancellation).
     */
    Task* submitTask(std::function<void()> func, const QString& name = QString(), Task::Priority priority = Task::Priority::Normal);

    /**
     * @brief Cancel a specific task if it's still queued.
     * @param task The task to cancel.
     * @return True if cancellation was successful.
     */
    bool cancelTask(Task* task);

    /**
     * @brief Cancel a task by its ID if it's still queued.
     * @param taskId The ID of the task to cancel.
     * @return True if cancellation was successful.
     */
    bool cancelTaskById(quintptr taskId);

    /**
     * @brief Cancel all tasks currently in the queue (not running ones).
     * @return Number of tasks canceled.
     */
    int cancelAllQueuedTasks();

    /**
     * @brief Get the maximum number of threads in the pool.
     * @return Max thread count.
     */
    int maxThreadCount() const;

    /**
     * @brief Set the maximum number of threads in the pool.
     * @param count New max thread count.
     */
    void setMaxThreadCount(int count);

    /**
     * @brief Get the current number of active threads.
     * @return Active thread count.
     */
    int activeThreadCount() const;

    /**
     * @brief Get the number of tasks currently running.
     * @return Running task count.
     */
    int runningTaskCount() const;

    /**
     * @brief Get the number of tasks waiting in the queue.
     * @return Queued task count.
     */
    int queuedTaskCount() const;

    /**
     * @brief Get the total number of tasks ever submitted.
     * @return Total task count.
     */
    quint64 totalTasksSubmitted() const;

    /**
     * @brief Get the total number of tasks completed (finished or canceled).
     * @return Total completed count.
     */
    quint64 totalTasksCompleted() const;

    /**
     * @brief Wait for all currently running tasks to finish.
     * @param msecs Maximum time to wait in milliseconds (UINT_MAX for forever).
     */
    void waitForDone(int msecs = UINT_MAX);

    /**
     * @brief Clear completed tasks from internal tracking lists.
     * This helps manage memory if many short tasks are run.
     */
    void clearCompletedTasks();

    /**
     * @brief Get a list of all tracked tasks (queued, running, finished).
     * @return List of task pointers.
     */
    QList<Task*> allTasks() const;

    /**
     * @brief Get a list of tasks matching a specific state.
     * @param state The state to filter by.
     * @return List of task pointers.
     */
    QList<Task*> tasksByState(Task::State state) const;

    /**
     * @brief Get a task by its ID.
     * @param id The task ID.
     * @return Pointer to the task, or nullptr if not found.
     */
    Task* taskById(quintptr id) const;

signals:
    /**
     * @brief Emitted when a task state changes.
     * @param task The task that changed.
     * @param newState The new state of the task.
     */
    void taskStateChanged(QuantilyxDoc::Task* task, QuantilyxDoc::Task::State newState);

    /**
     * @brief Emitted when a task is added to the queue.
     * @param task The added task.
     */
    void taskQueued(QuantilyxDoc::Task* task);

    /**
     * @brief Emitted when a task starts executing.
     * @param task The started task.
     */
    void taskStarted(QuantilyxDoc::Task* task);

    /**
     * @brief Emitted when a task finishes executing.
     * @param task The finished task.
     */
    void taskFinished(QuantilyxDoc::Task* task);

    /**
     * @brief Emitted when the queue status (counts) changes.
     * @param queuedCount Number of queued tasks.
     * @param runningCount Number of running tasks.
     * @param activeThreadCount Number of active threads.
     */
    void queueStatusChanged(int queuedCount, int runningCount, int activeThreadCount);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_THREADPOOL_H