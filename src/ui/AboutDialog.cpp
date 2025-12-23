/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "AboutDialog.h"
#include "../core/Application.h" // To get app version, name, etc.
#include "../core/Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QTextBrowser>
#include <QPushButton>
#include <QTabWidget>
#include <QIcon>
#include <QDesktopServices>
#include <QUrl>
#include <QApplication>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QDebug>

namespace QuantilyxDoc {

class AboutDialog::Private {
public:
    Private(AboutDialog* q_ptr)
        : q(q_ptr) {}

    AboutDialog* q;

    // UI Elements
    QLabel* logoLabel;
    QLabel* titleLabel;
    QLabel* versionLabel;
    QLabel* copyrightLabel;
    QLabel* licenseLabel;
    QTextBrowser* descriptionBrowser;
    QTextBrowser* licenseBrowser;
    QTabWidget* detailsTabWidget;
    QTextBrowser* librariesBrowser;
    QTextBrowser* thirdPartyBrowser;
    QDialogButtonBox* buttonBox;
    QPushButton* okButton;
    QPushButton* websiteButton;
    QPushButton* repoButton;

    // Data
    QString appNameStr;
    QString appVersionStr;
    QString copyrightStr;
    QString licenseStr;
    QString descriptionStr;
    QIcon logoIcon;
    QString websiteUrlStr;
    QString repoUrlStr;
    QString authorStr;
    QString sloganStr;
    QList<LibraryInfo> librariesList;

    // Helper to create the main layout and widgets
    void createLayout();
    // Helper to populate text fields
    void populateText();
    // Helper to connect signals
    void connectSignals();
};

void AboutDialog::Private::createLayout()
{
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(q);

    // Top Section: Logo and Text
    QHBoxLayout* topLayout = new QHBoxLayout();

    // Logo
    logoLabel = new QLabel(q);
    logoLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    // logoLabel->setPixmap(logoIcon.pixmap(64, 64)); // Set pixmap if icon is loaded
    logoLabel->setPixmap(QIcon(":/icons/app_icon.png").pixmap(64, 64)); // Use a placeholder resource
    topLayout->addWidget(logoLabel);

    // Text Column
    QVBoxLayout* textLayout = new QVBoxLayout();

    titleLabel = new QLabel(q);
    titleLabel->setTextFormat(Qt::RichText); // Allow HTML for formatting
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    textLayout->addWidget(titleLabel);

    versionLabel = new QLabel(q);
    versionLabel->setAlignment(Qt::AlignLeft);
    textLayout->addWidget(versionLabel);

    copyrightLabel = new QLabel(q);
    copyrightLabel->setAlignment(Qt::AlignLeft);
    textLayout->addWidget(copyrightLabel);

    // Slogan
    QLabel* sloganLabel = new QLabel(q);
    sloganLabel->setText(sloganStr);
    sloganLabel->setAlignment(Qt::AlignLeft);
    sloganLabel->setStyleSheet("QLabel { font-style: italic; color: gray; }"); // Style the slogan
    textLayout->addWidget(sloganLabel);

    topLayout->addLayout(textLayout);
    topLayout->addStretch(); // Push content to the left
    mainLayout->addLayout(topLayout);

    // Description (scrollable)
    descriptionBrowser = new QTextBrowser(q);
    descriptionBrowser->setOpenExternalLinks(true); // Allow links in description
    descriptionBrowser->setMaximumHeight(80); // Limit height
    descriptionBrowser->setReadOnly(true);
    mainLayout->addWidget(descriptionBrowser);

    // License (scrollable)
    licenseLabel = new QLabel(tr("License:"), q);
    mainLayout->addWidget(licenseLabel);
    licenseBrowser = new QTextBrowser(q);
    licenseBrowser->setOpenExternalLinks(true);
    licenseBrowser->setMaximumHeight(100); // Limit height
    licenseBrowser->setReadOnly(true);
    mainLayout->addWidget(licenseBrowser);

    // Details Tabs
    detailsTabWidget = new QTabWidget(q);
    librariesBrowser = new QTextBrowser(q);
    librariesBrowser->setOpenExternalLinks(true);
    detailsTabWidget->addTab(librariesBrowser, tr("Libraries"));

    thirdPartyBrowser = new QTextBrowser(q);
    thirdPartyBrowser->setOpenExternalLinks(true);
    detailsTabWidget->addTab(thirdPartyBrowser, tr("Third-Party"));

    mainLayout->addWidget(detailsTabWidget);

    // Buttons
    buttonBox = new QDialogButtonBox(q);
    okButton = new QPushButton(tr("OK"), q);
    websiteButton = new QPushButton(tr("Visit Website"), q);
    repoButton = new QPushButton(tr("Source Code"), q);

    buttonBox->addButton(okButton, QDialogButtonBox::AcceptRole);
    buttonBox->addButton(websiteButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(repoButton, QDialogButtonBox::ActionRole);

    mainLayout->addWidget(buttonBox);

    // Set dialog properties
    q->setWindowTitle(tr("About %1").arg(appNameStr));
    q->setModal(true);
    q->setFixedSize(500, 600); // Fixed size, adjust as needed
}

void AboutDialog::Private::populateText()
{
    appNameStr = QApplication::applicationName();
    appVersionStr = QApplication::applicationVersion();
    copyrightStr = Application::copyrightNotice(); // Assuming Application class has this
    authorStr = Application::organizationName(); // Or a dedicated author field
    sloganStr = Application::applicationSlogan(); // Assuming Application class has this
    websiteUrlStr = Application::websiteUrl(); // Assuming Application class has this
    repoUrlStr = Application::repositoryUrl(); // Assuming Application class has this

    // Title
    titleLabel->setText(QString("<h2>%1</h2>").arg(appNameStr));

    // Version
    versionLabel->setText(tr("Version %1").arg(appVersionStr.isEmpty() ? "Unknown" : appVersionStr));

    // Copyright
    copyrightLabel->setText(copyrightStr);

    // Description
    descriptionStr = tr("A professional, open-source document editor focused on liberation and advanced features.");
    descriptionBrowser->setText(descriptionStr);

    // License Text (Example GPL V3)
    licenseStr = tr(
        "<p>This program is free software: you can redistribute it and/or modify "
        "it under the terms of the GNU General Public License as published by "
        "the Free Software Foundation, either version 3 of the License, or "
        "(at your option) any later version.</p>"
        "<p>This program is distributed in the hope that it will be useful, "
        "but WITHOUT ANY WARRANTY; without even the implied warranty of "
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
        "GNU General Public License for more details.</p>"
        "<p>You should have received a copy of the GNU General Public License "
        "along with this program. If not, see "
        "<a href=\"https://www.gnu.org/licenses/gpl-3.0.html\">https://www.gnu.org/licenses/gpl-3.0.html</a>.</p>"
    );
    licenseBrowser->setHtml(licenseStr);

    // Libraries Info
    QString libsText = "<h3>Libraries Used</h3><ul>";
    libsText += "<li><a href=\"https://www.qt.io/\">Qt Framework</a> (LGPL v3)</li>";
    libsText += "<li><a href=\"https://poppler.freedesktop.org/\">Poppler</a> (GPL/AGPL/MPL)</li>";
    libsText += "<li><a href=\"https://www.libsdl.org/\">SDL2</a> (zlib)</li>";
    libsText += "<li><a href=\"https://libzip.org/\">libzip</a> (BSD-3-Clause)</li>";
    libsText += "<li><a href=\"https://tesseract-ocr.github.io/\">Tesseract OCR</a> (Apache 2.0)</li>";
    libsText += "<li><a href=\"https://github.com/chriskohlhoff/asio\">Asio</a> (Boost Software License)</li>";
    libsText += "<li><a href=\"https://www.boost.org/\">Boost</a> (Boost Software License)</li>";
    // Add more as integrated
    libsText += "</ul>";
    librariesBrowser->setHtml(libsText);

    // Third Party (could be same as libraries or separate e.g. for icons/themes)
    QString thirdPartyText = tr("<h3>Third-Party Assets</h3><p>Includes icons from the Tango Desktop Project (Public Domain).</p>");
    thirdPartyBrowser->setHtml(thirdPartyText);

    LOG_DEBUG("AboutDialog: Populated text fields.");
}

void AboutDialog::Private::connectSignals()
{
    connect(okButton, &QPushButton::clicked, q, &QDialog::accept);
    connect(websiteButton, &QPushButton::clicked, [this]() {
        if (!websiteUrlStr.isEmpty()) {
            QDesktopServices::openUrl(QUrl(websiteUrlStr));
        } else {
            QMessageBox::information(q, tr("Info"), tr("Website URL is not set."));
        }
    });
    connect(repoButton, &QPushButton::clicked, [this]() {
        if (!repoUrlStr.isEmpty()) {
            QDesktopServices::openUrl(QUrl(repoUrlStr));
        } else {
            QMessageBox::information(q, tr("Info"), tr("Repository URL is not set."));
        }
    });
}

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
    , d(new Private(this))
{
    d->createLayout();
    d->populateText();
    d->connectSignals();

    LOG_INFO("AboutDialog initialized.");
}

AboutDialog::~AboutDialog()
{
    LOG_INFO("AboutDialog destroyed.");
}

void AboutDialog::setAppName(const QString& name)
{
    d->appNameStr = name;
    d->titleLabel->setText(QString("<h2>%1</h2>").arg(name));
    setWindowTitle(tr("About %1").arg(name));
}

QString AboutDialog::appName() const
{
    return d->appNameStr;
}

void AboutDialog::setAppVersion(const QString& version)
{
    d->appVersionStr = version;
    d->versionLabel->setText(tr("Version %1").arg(version));
}

QString AboutDialog::appVersion() const
{
    return d->appVersionStr;
}

void AboutDialog::setCopyright(const QString& copyright)
{
    d->copyrightStr = copyright;
    d->copyrightLabel->setText(copyright);
}

QString AboutDialog::copyright() const
{
    return d->copyrightStr;
}

void AboutDialog::setLicense(const QString& license)
{
    d->licenseStr = license;
    d->licenseBrowser->setHtml(license);
}

QString AboutDialog::license() const
{
    return d->licenseStr;
}

void AboutDialog::setDescription(const QString& description)
{
    d->descriptionStr = description;
    d->descriptionBrowser->setText(description);
}

QString AboutDialog::description() const
{
    return d->descriptionStr;
}

void AboutDialog::setLogo(const QIcon& icon)
{
    d->logoIcon = icon;
    d->logoLabel->setPixmap(icon.pixmap(64, 64)); // Or use a size from settings
}

QIcon AboutDialog::logo() const
{
    return d->logoIcon;
}

void AboutDialog::addLibrary(const QString& name, const QString& version, const QString& license, const QString& homepage, const QString& description)
{
    LibraryInfo info;
    info.name = name;
    info.version = version;
    info.license = license;
    info.homepage = homepage;
    info.description = description;
    d->librariesList.append(info);

    // Update the libraries text browser
    QString currentText = d->librariesBrowser->toHtml();
    QString newText = QString("<li><a href=\"%1\">%2</a> (%3) - %4</li>")
                          .arg(homepage, name, version, license);
    // This is a simplistic way to append. A better way is to rebuild the entire list.
    // For now, we'll rebuild.
    QString libsText = "<h3>Libraries Used</h3><ul>";
    for (const auto& lib : d->librariesList) {
        libsText += QString("<li><a href=\"%1\">%2</a> (%3) - %4</li>")
                        .arg(lib.homepage, lib.name, lib.version, lib.license);
    }
    libsText += "</ul>";
    d->librariesBrowser->setHtml(libsText);
    LOG_DEBUG("AboutDialog: Added library '" << name << "' to list.");
}

QList<LibraryInfo> AboutDialog::libraries() const
{
    return d->librariesList;
}

void AboutDialog::setLibraries(const QList<LibraryInfo>& libraries)
{
    d->librariesList = libraries;
    // Rebuild the libraries text browser content
    QString libsText = "<h3>Libraries Used</h3><ul>";
    for (const auto& lib : d->librariesList) {
        libsText += QString("<li><a href=\"%1\">%2</a> (%3) - %4</li>")
                        .arg(lib.homepage, lib.name, lib.version, lib.license);
    }
    libsText += "</ul>";
    d->librariesBrowser->setHtml(libsText);
    LOG_DEBUG("AboutDialog: Set " << libraries.size() << " libraries in list.");
}

QString AboutDialog::websiteUrl() const
{
    return d->websiteUrlStr;
}

void AboutDialog::setWebsiteUrl(const QString& url)
{
    d->websiteUrlStr = url;
}

QString AboutDialog::repositoryUrl() const
{
    return d->repoUrlStr;
}

void AboutDialog::setRepositoryUrl(const QString& url)
{
    d->repoUrlStr = url;
}

QString AboutDialog::author() const
{
    return d->authorStr;
}

void AboutDialog::setAuthor(const QString& author)
{
    d->authorStr = author;
}

QString AboutDialog::slogan() const
{
    return d->sloganStr;
}

void AboutDialog::setSlogan(const QString& slogan)
{
    d->sloganStr = slogan;
    // Update the slogan label if it exists
    // Find the slogan label in the layout and update its text.
    // This is cumbersome with the current setup. A better way is to store a pointer to it in Private.
    // QLabel* sloganLabel = ...; // Need to find or store pointer
    // sloganLabel->setText(slogan);
    // For now, we'll just store the value. It's set during populateText.
    LOG_WARN("AboutDialog::setSlogan: Slogan is set during initialization/populateText. Changing it here requires updating the QLabel manually.");
}

} // namespace QuantilyxDoc