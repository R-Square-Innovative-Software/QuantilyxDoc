# QuantilyxDoc

**Professional Open-Source Document Editor for Linux**

> *"Where innovation is the key to success"*

[![License: GPLv3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Version](https://img.shields.io/badge/version-1.0.0-green.svg)](https://github.com/R-Square-Innovative-Software/QuantilyxDoc/releases)
[![Platform](https://img.shields.io/badge/platform-Linux-orange.svg)](https://github.com/R-Square-Innovative-Software/QuantilyxDoc)

---

## 📋 Overview

QuantilyxDoc is a powerful, feature-rich document editor designed to give you complete control over your PDF and document files. Built on the principle of **document liberation**, QuantilyxDoc helps you view, edit, convert, and manage documents without artificial restrictions.

### 🎯 Key Philosophy

**We believe users should have complete control over documents they legally own.**

- ❌ No copy protection
- ❌ No print restrictions  
- ❌ No artificial limitations
- ✅ Full document liberation
- ✅ Password removal tools
- ✅ Complete transparency

---

## ✨ Features

### 📄 Universal Document Support
- **15+ formats**: PDF, EPUB, DjVu, CBR/CBZ, PostScript, XPS, CHM, Markdown, Mobi, Images, 2D CAD, and more
- Seamless format conversion
- High-quality rendering

### ✏️ Advanced Editing
- Rich text editing with Office suite features
- Advanced typography and OpenType support
- Drawing tools and shape library
- Form creation and editing
- 18+ annotation types
- Table creation and manipulation

### 🔍 OCR & Recognition
- Dual OCR engines (Tesseract & PaddleOCR)
- 50+ language support
- Batch OCR processing
- Smart content recognition
- Auto-enhancement of scanned documents

### 🔓 Security & Liberation
- **Remove document passwords** (owner & user)
- Document sanitization and privacy protection
- Digital signatures and certificates
- Redaction with verification
- Audit trail system

### ⚙️ Automation
- Visual workflow builder
- Watched folders with smart actions
- Batch processing for any operation
- Macro recording and playback
- Lua/Python scripting support

### 🔄 Conversion
- Convert between 50+ formats
- Conversion profiles for common scenarios
- Batch conversion
- Quality optimization
- Universal converter plugin

### 🎨 Design & Templates
- Professional document templates
- Brand kit manager
- Advanced graphics tools
- Typography management
- Style guides

### 📚 Document Management
- Virtual library with smart collections
- Advanced tagging and metadata
- Document relationships and linking
- Version control with Git-like features
- Full-text search across all documents

---

## 🚀 Getting Started

### System Requirements

**Minimum:**
- Debian 9 / Ubuntu 16.04 or newer
- 2 GB RAM
- 500 MB disk space
- 1024x768 display

**Recommended:**
- Debian 12 / Ubuntu 22.04 or newer
- 8 GB RAM
- 2 GB disk space
- 1920x1080 display
- Multi-core processor

### Dependencies

```bash
# Debian/Ubuntu
sudo apt-get install \
    build-essential \
    cmake \
    qtbase5-dev \
    qtchooser \
    qt5-qmake \
    qtbase5-dev-tools \
    qttools5-dev \
    libpoppler-qt5-dev \
    libssl-dev \
    zlib1g-dev \
    tesseract-ocr \
    libtesseract-dev

# Optional dependencies
sudo apt-get install \
    libdjvulibre-dev \
    libspectre-dev \
    libarchive-dev \
    libchm-dev
```

### Building from Source

```bash
# Clone the repository
git clone https://github.com/R-Square-Innovative-Software/QuantilyxDoc.git
cd QuantilyxDoc

# Build
./build.sh

# Install
sudo ./build.sh --install

# Run
quantilyxdoc
```

### Build Options

```bash
./build.sh --help              # Show all options
./build.sh --debug             # Debug build
./build.sh --clean --install   # Clean, build, and install
./build.sh --legacy            # Build for Debian 9
./build.sh -j 8                # Build with 8 parallel jobs
```

---

## 📖 Documentation

- **User Manual**: [docs/user-manual/index.html](docs/user-manual/index.html)
- **Configuration Reference**: [docs/user-manual/reference/configuration-reference.html](docs/user-manual/reference/configuration-reference.html)
- **Keyboard Shortcuts**: [docs/user-manual/reference/keyboard-shortcuts.html](docs/user-manual/reference/keyboard-shortcuts.html)
- **API Documentation**: [docs/developer/api/index.html](docs/developer/api/index.html)
- **Plugin Development**: [docs/developer/plugins/index.html](docs/developer/plugins/index.html)

### Multi-Language Support

Documentation available in:
- 🇬🇧 English
- 🇫🇷 Français  
- 🇩🇪 Deutsch
- 🇨🇳 中文
- 🇪🇸 Español
- 🇷🇺 Русский

---

## ⚙️ Configuration

QuantilyxDoc is **completely configurable** with 500+ settings:

```bash
# Configuration file location
~/.config/quantilyxdoc/quantilyxdoc.ini

# Cache and logs
~/.cache/quantilyxdoc/

# Data directory
~/.local/share/quantilyxdoc/
```

**Preferences Dialog**: Edit → Preferences (Ctrl+,)

See the [Complete Configuration Reference](docs/user-manual/reference/configuration-reference.html) for all settings.

---

## 🔌 Plugins

QuantilyxDoc supports optional plugins:

- **AI Assistant** - Summarization, Q&A, translation
- **Advanced OCR** - Handwriting, formulas
- **Data Extraction** - Invoices, resumes, forms
- **Version Control** - Git-like versioning
- **Accessibility** - WCAG checker, auto-fixer
- **Forensics** - Tamper detection, author attribution
- **Research Tools** - Citations, bibliography
- **Universal Converter** - 50+ format conversion
- **Mobile Companion** - Remote control, sync

Install plugins via: **Tools → Plugin Manager**

---

## 🛠️ Development

### Project Structure

```
QuantilyxDoc/
├── src/                  # Source code
├── resources/            # Images, translations, themes
├── plugins/              # Optional plugins
├── docs/                 # Documentation
├── tests/                # Unit tests
├── build-scripts/        # Build automation
├── cmake/                # CMake modules
├── debian/               # Debian packaging
└── rpm/                  # RPM packaging
```

### Building Packages

```bash
# DEB package
./build-scripts/package-deb.sh

# RPM package
./build-scripts/package-rpm.sh

# AppImage
./build-scripts/package-appimage.sh

# Flatpak
./build-scripts/package-flatpak.sh

# TAR.GZ
./build-scripts/package-tar.sh
```

### Running Tests

```bash
cd build
ctest --output-on-failure
```

---

## 🤝 Contributing

We welcome contributions! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Ways to Contribute

- 🐛 Report bugs
- 💡 Suggest features
- 📝 Improve documentation
- 🌍 Translate to new languages
- 🔌 Create plugins
- 💻 Submit code improvements

### Code of Conduct

This project follows the [Contributor Covenant Code of Conduct](CODE_OF_CONDUCT.md).

---

## 📜 License

QuantilyxDoc is licensed under the **GNU General Public License v3.0**.

See [LICENSE](LICENSE) for the full license text.

### Key Points:
- ✅ Free to use, modify, and distribute
- ✅ Source code must remain open
- ✅ Derivatives must use GPLv3
- ✅ No warranty provided

---

## 👥 Authors & Credits

**Developer**: R² Innovative Software Development Team  
**Company**: R² Innovative Software  
**Motto**: "Where innovation is the key to success"  
**Website**: [github.com/R-Square-Innovative-Software](https://github.com/R-Square-Innovative-Software)

### Third-Party Libraries

- **Qt5** - Application framework (LGPL)
- **Poppler** - PDF rendering (GPL)
- **Tesseract** - OCR engine (Apache 2.0)
- **OpenSSL** - Cryptography (Apache 2.0)
- And many more... see [CREDITS.md](CREDITS.md)

---

## 📞 Support

- 🐛 **Bug Reports**: [GitHub Issues](https://github.com/R-Square-Innovative-Software/QuantilyxDoc/issues)
- 💬 **Discussions**: [GitHub Discussions](https://github.com/R-Square-Innovative-Software/QuantilyxDoc/discussions)
- 📧 **Email**: support@rsquaretech.example
- 📖 **Documentation**: [User Manual](docs/user-manual/index.html)
- ❓ **FAQ**: [docs/user-manual/faq/index.html](docs/user-manual/faq/index.html)

---

## 🗺️ Roadmap

### Version 1.0 (Current)
- ✅ Core PDF editing
- ✅ 15+ format support
- ✅ OCR integration
- ✅ Password removal
- ✅ 500+ configuration options

### Version 1.1 (Future)
- Advanced search improvements
- Enhanced automation workflows
- More plugins
- Performance optimizations

### Version 1.5 (Future)
- Cloud collaboration (optional)
- Mobile companion app
- Advanced AI features
- Professional publishing tools

### Version 2.0 (Vision)
- Major UI refresh
- Rewritten rendering engine
- GPU acceleration
- Real-time collaboration

---

## 📊 Statistics

- **Lines of Code**: ~250,000
- **Features**: 180+
- **Configuration Options**: 500+
- **Supported Formats**: 15+
- **Languages**: 6
- **Plugins**: 12

---

## ⭐ Star History

If you find QuantilyxDoc useful, please consider giving it a star! ⭐

---

## 📸 Screenshots

![Main Interface](docs/images/screenshots/main-window.png)
*Main interface with dockable panels*

![PDF Editing](docs/images/screenshots/pdf-editing.png)
*Advanced PDF editing capabilities*

![OCR Processing](docs/images/screenshots/ocr.png)
*Batch OCR with dual engine support*

![Password Removal](docs/images/screenshots/password-removal.png)
*Document liberation - password removal*

---

## 🎯 Mission

Our mission is to create the most powerful, user-friendly, and unrestricted document editor for Linux. We believe in **document liberation** - users should have complete control over documents they legally own.

---

**Built with ❤️ by R² Innovative Software**

*"Where innovation is the key to success"*
