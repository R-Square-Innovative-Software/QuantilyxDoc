/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_MACRORECORDER_H
#define QUANTILYX_MACRORECORDER_H

#include <QObject>
#include <QList>
#include <QVariant>
#include <QDateTime>
#include <QMutex>
#include <memory>

namespace QuantilyxDoc {

/**
 * @brief Represents a single action/event recorded by the macro recorder.
 */
struct RecordedAction {
    QString actionType;       // e.g., "File.Open", "Edit.Undo", "View.ZoomIn", "Document.GoToPage"
    QDateTime timestamp;      // When the action occurred
    QVariantMap parameters;   // Arguments passed to the action (e.g., filename, page index, zoom level)
    QString description;      // Human-readable description of the action
};

/**
 * @brief Records and plays back sequences of user actions (macros).
 * 
 * Captures user interactions like menu clicks, toolbar button presses, keyboard shortcuts,
 * and document modifications, allowing them to be replayed later.
 */
class MacroRecorder : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     * @param parent Parent object.
     */
    explicit MacroRecorder(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~MacroRecorder() override;

    /**
     * @brief Get singleton instance.
     * @return Reference to the global MacroRecorder instance.
     */
    static MacroRecorder& instance();

    /**
     * @brief Start recording user actions.
     */
    void startRecording();

    /**
     * @brief Stop recording user actions.
     */
    void stopRecording();

    /**
     * @brief Check if the recorder is currently recording.
     * @return True if recording.
     */
    bool isRecording() const;

    /**
     * @brief Play back the currently recorded macro sequence.
     * Executes the recorded actions in order.
     */
    void playBack();

    /**
     * @brief Pause playback of the current macro.
     */
    void pausePlayback();

    /**
     * @brief Resume playback of the current macro.
     */
    void resumePlayback();

    /**
     * @brief Stop playback of the current macro.
     */
    void stopPlayback();

    /**
     * @brief Check if the recorder is currently playing back a macro.
     * @return True if playing back.
     */
    bool isPlayingBack() const;

    /**
     * @brief Get the list of recorded actions.
     * @return List of RecordedAction structures.
     */
    QList<RecordedAction> recordedActions() const;

    /**
     * @brief Clear the current list of recorded actions.
     */
    void clearRecording();

    /**
     * @brief Load a macro from a file.
     * @param filePath Path to the macro file (e.g., JSON).
     * @return True if loading was successful.
     */
    bool loadMacroFromFile(const QString& filePath);

    /**
     * @brief Save the current recording to a file.
     * @param filePath Path to save the macro file (e.g., JSON).
     * @return True if saving was successful.
     */
    bool saveMacroToFile(const QString& filePath) const;

    /**
     * @brief Get the name of the currently loaded macro.
     * @return Macro name string.
     */
    QString currentMacroName() const;

    /**
     * @brief Set the name of the current macro.
     * @param name Macro name string.
     */
    void setCurrentMacroName(const QString& name);

    /**
     * @brief Get the duration of the currently recorded macro.
     * @return Duration in milliseconds.
     */
    qint64 macroDuration() const;

    /**
     * @brief Set the playback speed multiplier (e.g., 1.0 for normal, 2.0 for double speed).
     * @param speed Speed multiplier.
     */
    void setPlaybackSpeed(double speed);

    /**
     * @brief Get the current playback speed multiplier.
     * @return Speed multiplier.
     */
    double playbackSpeed() const;

    /**
     * @brief Check if the macro should loop when playback reaches the end.
     * @return True if looping is enabled.
     */
    bool isLooping() const;

    /**
     * @brief Enable or disable looping of the macro during playback.
     * @param looping Whether to loop.
     */
    void setLooping(bool looping);

    /**
     * @brief Get the list of supported macro file formats (e.g., "json", "qtx").
     * @return List of format strings.
     */
    QStringList supportedMacroFormats() const;

signals:
    /**
     * @brief Emitted when recording starts.
     */
    void recordingStarted();

    /**
     * @brief Emitted when recording stops.
     */
    void recordingStopped();

    /**
     * @brief Emitted when a new action is recorded.
     * @param action The recorded action.
     */
    void actionRecorded(const QuantilyxDoc::RecordedAction& action);

    /**
     * @brief Emitted when playback starts.
     */
    void playbackStarted();

    /**
     * @brief Emitted when playback pauses.
     */
    void playbackPaused();

    /**
     * @brief Emitted when playback resumes.
     */
    void playbackResumed();

    /**
     * @brief Emitted when playback stops (either naturally or via stop command).
     */
    void playbackStopped();

    /**
     * @brief Emitted when the list of recorded actions changes.
     */
    void recordedActionsChanged();

    /**
     * @brief Emitted when a macro is loaded from a file.
     * @param filePath Path to the loaded file.
     */
    void macroLoaded(const QString& filePath);

    /**
     * @brief Emitted when a macro is saved to a file.
     * @param filePath Path to the saved file.
     */
    void macroSaved(const QString& filePath);

    /**
     * @brief Emitted periodically during playback to indicate progress.
     * @param progress Progress percentage (0-100).
     */
    void playbackProgress(int progress);

private:
    class Private;
    std::unique_ptr<Private> d;

    // Helper to register actions from the application's event system
    void registerAction(const QString& type, const QVariantMap& params);
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_MACRORECORDER_H