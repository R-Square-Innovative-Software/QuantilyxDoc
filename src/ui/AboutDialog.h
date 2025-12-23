/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_ABOUTDIALOG_H
#define QUANTILYX_ABOUTDIALOG_H

#include <QDialog>
#include <QList>
#include <QPair>
#include <memory>

namespace QuantilyxDoc {

/**
 * @brief Information about a third-party library used by the application.
 */
struct LibraryInfo {
    QString name;           // Name of the library (e.g., "Qt", "Poppler", "Zlib")
    QString version;        // Version string (e.g., "5.15.2", "22.03.0")
    QString license;        // License type (e.g., "LGPL v3", "GPL v2", "MIT")
    QString homepage;       // Homepage URL
    QString description;    // Brief description of the library's role
};

/**
 * @brief Standard "About" dialog displaying application information.
 * 
 * Shows the application name, version, copyright, license, description,
 * and a list of third-party libraries used.
 */
class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     * @param parent Parent widget (usually the main window).
     */
    explicit AboutDialog(QWidget* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~AboutDialog() override;

    /**
     * @brief Set the application name displayed in the dialog.
     * @param name Application name string.
     */
    void setAppName(const QString& name);

    /**
     * @brief Get the application name displayed in the dialog.
     * @return Application name string.
     */
    QString appName() const;

    /**
     * @brief Set the application version displayed in the dialog.
     * @param version Version string (e.g., "1.0.0", "v2.1.3-alpha").
     */
    void setAppVersion(const QString& version);

    /**
     * @brief Get the application version displayed in the dialog.
     * @return Version string.
     */
    QString appVersion() const;

    /**
     * @brief Set the copyright text displayed in the dialog.
     * @param copyright Copyright string (e.g., "Copyright (C) 2025 R² Innovative Software").
     */
    void setCopyright(const QString& copyright);

    /**
     * @brief Get the copyright text displayed in the dialog.
     * @return Copyright string.
     */
    QString copyright() const;

    /**
     * @brief Set the main license text displayed in the dialog.
     * @param license License text string (can be plain text or HTML).
     */
    void setLicense(const QString& license);

    /**
     * @brief Get the main license text displayed in the dialog.
     * @return License text string.
     */
    QString license() const;

    /**
     * @brief Set the application description displayed in the dialog.
     * @param description Description string.
     */
    void setDescription(const QString& description);

    /**
     * @brief Get the application description displayed in the dialog.
     * @return Description string.
     */
    QString description() const;

    /**
     * @brief Set the application logo/icon displayed in the dialog.
     * @param icon Logo QIcon.
     */
    void setLogo(const QIcon& icon);

    /**
     * @brief Get the application logo/icon displayed in the dialog.
     * @return Logo QIcon.
     */
    QIcon logo() const;

    /**
     * @brief Add information about a third-party library used by the application.
     * @param name Library name.
     * @param version Library version.
     * @param license Library license.
     * @param homepage Library homepage URL.
     * @param description Library description.
     */
    void addLibrary(const QString& name, const QString& version, const QString& license, const QString& homepage, const QString& description);

    /**
     * @brief Get the list of registered third-party libraries.
     * @return List of LibraryInfo structures.
     */
    QList<LibraryInfo> libraries() const;

    /**
     * @brief Set the list of third-party libraries used by the application.
     * Replaces the current list.
     * @param libraries List of LibraryInfo structures.
     */
    void setLibraries(const QList<LibraryInfo>& libraries);

    /**
     * @brief Get the application's website URL.
     * @return Website URL string.
     */
    QString websiteUrl() const;

    /**
     * @brief Set the application's website URL.
     * @param url Website URL string.
     */
    void setWebsiteUrl(const QString& url);

    /**
     * @brief Get the application's repository URL (e.g., GitHub).
     * @return Repository URL string.
     */
    QString repositoryUrl() const;

    /**
     * @brief Set the application's repository URL (e.g., GitHub).
     * @param url Repository URL string.
     */
    void setRepositoryUrl(const QString& url);

    /**
     * @brief Get the application's author or organization name.
     * @return Author/Organization name string.
     */
    QString author() const;

    /**
     * @brief Set the application's author or organization name.
     * @param author Author/Organization name string.
     */
    void setAuthor(const QString& author);

    /**
     * @brief Get the application's slogan or tagline.
     * @return Slogan string.
     */
    QString slogan() const;

    /**
     * @brief Set the application's slogan or tagline.
     * @param slogan Slogan string.
     */
    void setSlogan(const QString& slogan);

signals:
    /**
     * @brief Emitted when a link (e.g., to website, repository, library homepage) is clicked.
     * @param url The URL that was clicked.
     */
    void linkClicked(const QUrl& url);

private:
    class Private;
    std::unique_ptr<Private> d;

    void createLayout();
    void populateInfo();
    void connectSignals();
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_ABOUTDIALOG_H