/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "MacroRecorder.h"
#include "../core/Logger.h"
#include <QList>
#include <QVariantMap>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QDebug>

namespace QuantilyxDoc {

class MacroRecorder::Private {
public:
    Private(MacroRecorder* q_ptr)
        : q(q_ptr), recording(false), playingBack(false), looping(false), playbackSpeedMultiplier(1.0) {}

    MacroRecorder* q;
    mutable QMutex mutex; // Protect access to the action list and state variables
    bool recording;
    bool playingBack;
    bool looping;
    double playbackSpeedMultiplier;
    QString currentMacroNameStr;
    QList<RecordedAction> actions;
    QDateTime recordingStartTime;
    QDateTime playbackStartTime;

    // Helper to convert RecordedAction to/from JSON
    QJsonObject actionToJson(const RecordedAction& action) const {
        QJsonObject obj;
        obj["type"] = action.actionType;
        obj["timestamp"] = action.timestamp.toString(Qt::ISODateWithMs); // Or store relative time from start?
        obj["parameters"] = QJsonObject::fromVariantMap(action.parameters);
        obj["description"] = action.description;
        return obj;
    }

    RecordedAction jsonToAction(const QJsonObject& obj) const {
        RecordedAction action;
        action.actionType = obj["type"].toString();
        action.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODateWithMs);
        action.parameters = obj["parameters"].toObject().toVariantMap();
        action.description = obj["description"].toString();
        return action;
    }
};

// Static instance pointer
MacroRecorder* MacroRecorder::s_instance = nullptr;

MacroRecorder& MacroRecorder::instance()
{
    if (!s_instance) {
        s_instance = new MacroRecorder();
    }
    return *s_instance;
}

MacroRecorder::MacroRecorder(QObject* parent)
    : QObject(parent)
    , d(new Private(this))
{
    LOG_INFO("MacroRecorder created.");
}

MacroRecorder::~MacroRecorder()
{
    if (isRecording()) {
        stopRecording(); // Ensure recording is stopped on destruction
    }
    if (isPlayingBack()) {
        stopPlayback(); // Ensure playback is stopped on destruction
    }
    LOG_INFO("MacroRecorder destroyed.");
}

void MacroRecorder::startRecording()
{
    QMutexLocker locker(&d->mutex);
    if (d->recording) {
        LOG_WARN("MacroRecorder: Already recording.");
        return;
    }

    d->actions.clear(); // Start fresh
    d->recording = true;
    d->recordingStartTime = QDateTime::currentDateTime();
    LOG_INFO("MacroRecorder: Started recording.");
    emit recordingStarted();
}

void MacroRecorder::stopRecording()
{
    QMutexLocker locker(&d->mutex);
    if (!d->recording) {
        LOG_WARN("MacroRecorder: Not currently recording.");
        return;
    }

    d->recording = false;
    LOG_INFO("MacroRecorder: Stopped recording. Recorded " << d->actions.size() << " actions.");
    emit recordingStopped();
    emit recordedActionsChanged();
}

bool MacroRecorder::isRecording() const
{
    QMutexLocker locker(&d->mutex);
    return d->recording;
}

void MacroRecorder::playBack()
{
    QMutexLocker locker(&d->mutex);
    if (d->playingBack) {
        LOG_WARN("MacroRecorder: Playback already in progress.");
        return;
    }

    if (d->actions.isEmpty()) {
        LOG_WARN("MacroRecorder: No actions to play back.");
        return;
    }

    d->playingBack = true;
    d->playbackStartTime = QDateTime::currentDateTime();
    LOG_INFO("MacroRecorder: Started playback of " << d->actions.size() << " actions.");
    emit playbackStarted();

    // This is where the actual playback logic happens.
    // It needs to iterate through d->actions and execute them.
    // This is complex as it requires mapping action types ("File.Open") back to actual application functions.
    // A common approach is to have a central "Action Registry" where each action type is associated with a callable function.
    // For this example, we'll use a simplified approach with a lookup table or signal emission.

    // Example using signals (requires UI/mainwindow to connect and handle them):
    int totalActions = d->actions.size();
    int currentActionIndex = 0;
    for (const auto& action : d->actions) {
        if (!d->playingBack) break; // Check if playback was stopped during iteration

        // Calculate delay based on timestamps and playback speed
        if (currentActionIndex > 0) {
            qint64 prevTimestampMs = d->actions[currentActionIndex - 1].timestamp.toMSecsSinceEpoch();
            qint64 currentTimestampMs = action.timestamp.toMSecsSinceEpoch();
            qint64 deltaTimeMs = currentTimestampMs - prevTimestampMs;
            int delayMs = static_cast<int>(deltaTimeMs / d->playbackSpeedMultiplier);
            if (delayMs > 0) {
                QThread::msleep(delayMs); // Sleep for the calculated delay (blocks thread!)
            }
        }

        // Emit signal for the action
        emit actionTriggered(action.actionType, action.parameters);

        // Emit progress
        int progress = (currentActionIndex + 1) * 100 / totalActions;
        emit playbackProgress(progress);

        currentActionIndex++;
    }

    // Playback finished (or was stopped)
    d->playingBack = false;
    LOG_INFO("MacroRecorder: Finished playback.");
    emit playbackStopped();
}

void MacroRecorder::pausePlayback()
{
    QMutexLocker locker(&d->mutex);
    if (d->playingBack) {
        // Pausing is complex with a simple loop. Requires more sophisticated scheduling.
        // For now, just log.
        LOG_WARN("MacroRecorder::pausePlayback: Not implemented with current playback mechanism.");
        // d->playingBack = false; // Would effectively stop it
        // emit playbackPaused();
    }
}

void MacroRecorder::resumePlayback()
{
    QMutexLocker locker(&d->mutex);
    if (!d->playingBack) {
        LOG_WARN("MacroRecorder::resumePlayback: Not paused.");
        // This would require storing the state at which it was paused.
        LOG_WARN("MacroRecorder::resumePlayback: Not implemented with current playback mechanism.");
    }
}

void MacroRecorder::stopPlayback()
{
    QMutexLocker locker(&d->mutex);
    if (d->playingBack) {
        d->playingBack = false;
        LOG_INFO("MacroRecorder: Playback stopped by user.");
        // The loop in playBack() will check d->playingBack and exit.
        // emit playbackStopped(); // This is emitted at the end of playBack()
    }
}

bool MacroRecorder::isPlayingBack() const
{
    QMutexLocker locker(&d->mutex);
    return d->playingBack;
}

QList<RecordedAction> MacroRecorder::recordedActions() const
{
    QMutexLocker locker(&d->mutex);
    return d->actions; // Returns a copy
}

void MacroRecorder::clearRecording()
{
    QMutexLocker locker(&d->mutex);
    if (d->recording) {
        LOG_WARN("MacroRecorder::clearRecording: Cannot clear while recording.");
        return;
    }
    d->actions.clear();
    LOG_DEBUG("MacroRecorder: Cleared recorded actions list.");
    emit recordedActionsChanged();
}

bool MacroRecorder::loadMacroFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR("MacroRecorder: Failed to open macro file for reading: " << filePath << ", Error: " << file.errorString());
        return false;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR("MacroRecorder: Failed to parse JSON macro file: " << filePath << ", Error: " << error.errorString());
        return false;
    }

    if (!doc.isArray()) {
        LOG_ERROR("MacroRecorder: Macro file JSON root is not an array: " << filePath);
        return false;
    }

    QMutexLocker locker(&d->mutex);
    d->actions.clear();
    QJsonArray array = doc.array();
    for (const auto& value : array) {
        if (value.isObject()) {
            d->actions.append(d->jsonToAction(value.toObject()));
        } else {
            LOG_WARN("MacroRecorder: Skipping non-object entry in macro file: " << filePath);
        }
    }

    QFileInfo fileInfo(filePath);
    d->currentMacroNameStr = fileInfo.baseName();
    LOG_INFO("MacroRecorder: Loaded macro from file: " << filePath << ", Actions: " << d->actions.size());
    emit macroLoaded(filePath);
    emit recordedActionsChanged();
    return true;
}

bool MacroRecorder::saveMacroToFile(const QString& filePath) const
{
    QMutexLocker locker(&d->mutex);
    QJsonArray jsonArray;
    for (const auto& action : d->actions) {
        jsonArray.append(d->actionToJson(action));
    }

    QJsonDocument doc(jsonArray);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR("MacroRecorder: Failed to open macro file for writing: " << filePath << ", Error: " << file.errorString());
        return false;
    }

    qint64 bytesWritten = file.write(doc.toJson());
    bool success = (bytesWritten == doc.toJson().size());
    if (success) {
        LOG_INFO("MacroRecorder: Saved macro to file: " << filePath << ", Actions: " << d->actions.size());
        emit macroSaved(filePath);
    } else {
        LOG_ERROR("MacroRecorder: Failed to write macro file: " << filePath << ", Bytes written: " << bytesWritten);
    }
    return success;
}

QString MacroRecorder::currentMacroName() const
{
    QMutexLocker locker(&d->mutex);
    return d->currentMacroNameStr;
}

void MacroRecorder::setCurrentMacroName(const QString& name)
{
    QMutexLocker locker(&d->mutex);
    if (d->currentMacroNameStr != name) {
        d->currentMacroNameStr = name;
        LOG_DEBUG("MacroRecorder: Macro name set to '" << name << "'");
    }
}

qint64 MacroRecorder::macroDuration() const
{
    QMutexLocker locker(&d->mutex);
    if (d->actions.isEmpty()) return 0;

    QDateTime firstTime = d->actions.first().timestamp;
    QDateTime lastTime = d->actions.last().timestamp;
    return firstTime.msecsTo(lastTime);
}

void MacroRecorder::setPlaybackSpeed(double speed)
{
    QMutexLocker locker(&d->mutex);
    if (d->playbackSpeedMultiplier != speed && speed > 0.0) {
        d->playbackSpeedMultiplier = speed;
        LOG_DEBUG("MacroRecorder: Playback speed set to " << speed << "x");
    }
}

double MacroRecorder::playbackSpeed() const
{
    QMutexLocker locker(&d->mutex);
    return d->playbackSpeedMultiplier;
}

bool MacroRecorder::isLooping() const
{
    QMutexLocker locker(&d->mutex);
    return d->looping;
}

void MacroRecorder::setLooping(bool looping)
{
    QMutexLocker locker(&d->mutex);
    if (d->looping != looping) {
        d->looping = looping;
        LOG_DEBUG("MacroRecorder: Looping set to " << (looping ? "enabled" : "disabled"));
    }
}

QStringList MacroRecorder::supportedMacroFormats() const
{
    return QStringList() << "json";
}

void MacroRecorder::registerAction(const QString& type, const QVariantMap& params)
{
    // This method would be called by the application framework when an action occurs.
    // It should only record if `isRecording()` is true.
    QMutexLocker locker(&d->mutex);
    if (d->recording) {
        RecordedAction action;
        action.actionType = type;
        action.timestamp = QDateTime::currentDateTime(); // Or relative time from recording start?
        action.parameters = params;
        action.description = QString("%1: %2").arg(type).arg(params.value("description", "Action").toString()); // Fallback description

        d->actions.append(action);
        LOG_DEBUG("MacroRecorder: Recorded action '" << type << "'");
        emit actionRecorded(action);
    }
}

} // namespace QuantilyxDoc