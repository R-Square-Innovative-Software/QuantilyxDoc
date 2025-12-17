# QuantilyxDoc Installation Guide

Complete installation instructions for all supported platforms.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [System Requirements](#system-requirements)
3. [Dependencies](#dependencies)
4. [Building from Source](#building-from-source)
5. [Binary Packages](#binary-packages)
6. [Legacy Systems](#legacy-systems)
7. [Post-Installation](#post-installation)
8. [Troubleshooting](#troubleshooting)
9. [Uninstallation](#uninstallation)

---

## Quick Start

### One-Line Install (Debian/Ubuntu)

```bash
# Download and run install script
curl -sSL https://raw.githubusercontent.com/R-Square-Innovative-Software/QuantilyxDoc/main/install.sh | bash
```

### Manual Install

```bash
git clone https://github.com/R-Square-Innovative-Software/QuantilyxDoc.git
cd QuantilyxDoc
./build.sh --clean --install
```

---

## System Requirements

### Minimum Requirements

- **OS**: Debian 9, Ubuntu 16.04, or equivalent
- **CPU**: Dual-core processor (x86_64)
- **RAM**: 2 GB
- **Disk**: 500 MB free space
- **Display**: 1024x768 resolution

### Recommended Requirements

- **OS**: Debian 12, Ubuntu 22.04, or newer
- **CPU**: Quad-core processor (x86_64)
- **RAM**: 8 GB
- **Disk**: 2 GB free space (including cache)
- **Display**: 1920x1080 or higher
- **GPU**: OpenGL 3.3+ support (for GPU acceleration)

### Supported Distributions

| Distribution | Version | Status |
|--------------|---------|--------|
| Debian | 9+ (Stretch) | ✅ Supported |
| Debian | 12+ (Bookworm) | ✅ Recommended |
| Ubuntu | 16.04+ (Xenial) | ✅ Supported |
| Ubuntu | 22.04+ (Jammy) | ✅ Recommended |
| Linux Mint | 18+ | ✅ Supported |
| Fedora | 30+ | ✅ Supported |
| openSUSE | Leap 15+ | ✅ Supported |
| Arch Linux | Rolling | ✅ Supported |

---

## Dependencies

### Build Dependencies

#### Debian/Ubuntu
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    qtchooser \
    qt5-qmake \
    qtbase5-dev \
    qttools5-dev \
    qttools5-dev-tools \
    libqt5svg5-dev \
    libqt5websockets5-dev \
    libpoppler-qt5-dev \
    libpoppler-cpp-dev \
    libssl-dev \
    zlib1g-dev \
    libtesseract-dev \
    tesseract-ocr \
    tesseract-ocr-eng
```

#### Fedora
```bash
sudo dnf install -y \
    gcc-c++ \
    cmake \
    git \
    qt5-qtbase-devel \
    qt5-qttools-devel \
    qt5-qtsvg-devel \
    qt5-qtwebsockets-devel \
    poppler-qt5-devel \
    openssl-devel \
    zlib-devel \
    poppler \
    poppler-utils \
    poppler-devel \
    poppler-data \
    tesseract-devel
```

#### Arch Linux
```bash
sudo pacman -S \
    base-devel \
    cmake \
    git \
    qt5-base \
    qt5-tools \
    qt5-svg \
    qt5-websockets \
    poppler-qt5 \
    openssl \
    zlib \
    tesseract \
    tesseract-data-eng
```

### Optional Dependencies

```bash
# DjVu support
sudo apt-get install libdjvulibre-dev

# PostScript support
sudo apt-get install libspectre-dev

# Archive support (CBR/CBZ)
sudo apt-get install libarchive-dev

# CHM support
sudo apt-get install libchm-dev

# Scanner support (SANE)
sudo apt-get install libsane-dev

# Spell checking
sudo apt-get install libhunspell-dev
```

### OCR Languages

Install additional OCR language data:

```bash
# French
sudo apt-get install tesseract-ocr-fra

# German
sudo apt-get install tesseract-ocr-deu

# Spanish
sudo apt-get install tesseract-ocr-spa

# Chinese (Simplified)
sudo apt-get install tesseract-ocr-chi-sim

# Russian
sudo apt-get install tesseract-ocr-rus

# All languages
sudo apt-get install tesseract-ocr-all
```

---

## Building from Source

### Standard Build

```bash
# 1. Clone the repository
git clone https://github.com/R-Square-Innovative-Software/QuantilyxDoc.git
cd QuantilyxDoc

# 2. Install dependencies (see above)

# 3. Build
./build.sh

# 4. Install
sudo ./build.sh --install
```

### Build Options

```bash
# Debug build
./build.sh --debug

# Clean build
./build.sh --clean

# Build without tests
./build.sh --no-tests

# Build without plugins
./build.sh --no-plugins

# Custom install prefix
./build.sh --prefix=/opt/quantilyxdoc

# Parallel build (8 jobs)
./build.sh -j 8

# All options
./build.sh --help
```

### Manual CMake Build

```bash
mkdir build
cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DBUILD_TESTS=ON \
    -DBUILD_PLUGINS=ON \
    -DENABLE_OCR_TESSERACT=ON

make -j$(nproc)
sudo make install
```

### Advanced Configuration

```bash
# Enable PaddleOCR (requires PaddleOCR installation)
cmake .. -DENABLE_OCR_PADDLEOCR=ON

# Disable GPU acceleration
cmake .. -DENABLE_GPU_ACCELERATION=OFF

# Custom Qt location
cmake .. -DCMAKE_PREFIX_PATH=/path/to/qt5
```

---

## Binary Packages

### DEB Package (Debian/Ubuntu)

```bash
# Download
wget https://github.com/R-Square-Innovative-Software/QuantilyxDoc/releases/download/v1.0.0/quantilyxdoc_1.0.0_amd64.deb

# Install
sudo dpkg -i quantilyxdoc_1.0.0_amd64.deb

# Fix dependencies if needed
sudo apt-get install -f
```

### RPM Package (Fedora/openSUSE)

```bash
# Fedora
sudo dnf install quantilyxdoc-1.0.0-1.x86_64.rpm

# openSUSE
sudo zypper install quantilyxdoc-1.0.0-1.x86_64.rpm
```

### AppImage (Universal)

```bash
# Download
wget https://github.com/R-Square-Innovative-Software/QuantilyxDoc/releases/download/v1.0.0/QuantilyxDoc-1.0.0-x86_64.AppImage

# Make executable
chmod +x QuantilyxDoc-1.0.0-x86_64.AppImage

# Run
./QuantilyxDoc-1.0.0-x86_64.AppImage
```

### Flatpak

```bash
# Add Flathub repository (if not already added)
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo

# Install
flatpak install flathub io.github.rsquare.QuantilyxDoc

# Run
flatpak run io.github.rsquare.QuantilyxDoc
```

### Snap (Future)

```bash
# Install
sudo snap install quantilyxdoc

# Run
quantilyxdoc
```

---

## Legacy Systems

### Building for Debian 9

```bash
# Use legacy build flag
./build.sh --legacy --install

# Or manually
cmake .. -DBUILD_LEGACY=ON
```

This will:
- Use compatible Qt version (5.7+)
- Disable features requiring newer libraries
- Use fallback implementations
- Optimize for older systems

### Compatibility Mode

If you encounter issues on older systems:

1. **Disable GPU acceleration**:
   ```bash
   ./build.sh --no-gpu
   ```

2. **Use software rendering**:
   ```bash
   export QT_XCB_GL_INTEGRATION=none
   quantilyxdoc
   ```

3. **Reduce memory usage**:
   Edit `~/.config/quantilyxdoc/quantilyxdoc.ini`:
   ```ini
   [Performance]
   max_memory_usage=1024
   page_cache_size=20
   ```

---

## Post-Installation

### First Run

After installation, launch QuantilyxDoc:

```bash
quantilyxdoc
```

The first run will:
1. Show welcome screen
2. Create configuration directory
3. Initialize database
4. Scan for OCR languages
5. Load plugins

### Desktop Integration

QuantilyxDoc should appear in your application menu. If not:

```bash
# Update desktop database
update-desktop-database ~/.local/share/applications/

# For system-wide installation
sudo update-desktop-database /usr/share/applications/
```

### File Associations

To set QuantilyxDoc as default for PDFs:

```bash
xdg-mime default quantilyxdoc.desktop application/pdf
```

### Configuration

Default configuration location:
```
~/.config/quantilyxdoc/quantilyxdoc.ini
```

Cache and logs:
```
~/.cache/quantilyxdoc/
```

User data:
```
~/.local/share/quantilyxdoc/
```

### Installing Plugins

```bash
# Via UI
Tools → Plugin Manager → Available Plugins

# Manual installation
cp plugin.so ~/.local/share/quantilyxdoc/plugins/
```

---

## Troubleshooting

### Build Issues

#### Qt not found
```bash
# Install Qt5
sudo apt-get install qt5-default qtbase5-dev

# Or specify Qt location
cmake .. -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/qt5
```

#### Poppler not found
```bash
# Install Poppler development files
sudo apt-get install libpoppler-qt5-dev libpoppler-cpp-dev
```

#### CMake version too old
```bash
# Install newer CMake from Kitware repository
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | sudo apt-key add -
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt-get update
sudo apt-get install cmake
```

### Runtime Issues

#### Application won't start
```bash
# Check dependencies
ldd /usr/local/bin/quantilyxdoc

# Run with debug output
quantilyxdoc --debug

# Check logs
tail -f ~/.cache/quantilyxdoc/logs/quantilyxdoc.log
```

#### Rendering issues
```bash
# Disable GPU acceleration
quantilyxdoc --no-gpu

# Or in config file
[Performance]
use_gpu=false
```

#### OCR not working
```bash
# Check Tesseract installation
tesseract --version

# Check language data
ls /usr/share/tesseract-ocr/4.00/tessdata/

# Install missing languages
sudo apt-get install tesseract-ocr-[language-code]
```

### Permission Issues

```bash
# Fix configuration directory permissions
chmod -R 755 ~/.config/quantilyxdoc/
chmod -R 755 ~/.cache/quantilyxdoc/
chmod -R 755 ~/.local/share/quantilyxdoc/
```

### Library Conflicts

```bash
# Check for library conflicts
LD_DEBUG=libs quantilyxdoc 2>&1 | grep -i error

# Force use system libraries
LD_LIBRARY_PATH=/usr/lib quantilyxdoc
```

---

## Uninstallation

### From Source

```bash
cd QuantilyxDoc/build
sudo make uninstall

# Or manually remove files
sudo rm /usr/local/bin/quantilyxdoc
sudo rm -rf /usr/local/share/quantilyxdoc
sudo rm /usr/share/applications/quantilyxdoc.desktop
```

### DEB Package

```bash
sudo apt-get remove quantilyxdoc
sudo apt-get purge quantilyxdoc  # Also remove config files
```

### RPM Package

```bash
sudo dnf remove quantilyxdoc
```

### Flatpak

```bash
flatpak uninstall io.github.rsquare.QuantilyxDoc
```

### Remove User Data

```bash
# Configuration
rm -rf ~/.config/quantilyxdoc/

# Cache
rm -rf ~/.cache/quantilyxdoc/

# User data
rm -rf ~/.local/share/quantilyxdoc/
```

---

## Getting Help

- 📖 **User Manual**: [docs/user-manual/index.html](docs/user-manual/index.html)
- 🐛 **Bug Reports**: [GitHub Issues](https://github.com/R-Square-Innovative-Software/QuantilyxDoc/issues)
- 💬 **Community**: [GitHub Discussions](https://github.com/R-Square-Innovative-Software/QuantilyxDoc/discussions)
- 📧 **Email**: support@rsquaretech.example

---

## Development Installation

For developers who want to contribute:

```bash
# Clone with submodules
git clone --recursive https://github.com/R-Square-Innovative-Software/QuantilyxDoc.git

# Build in debug mode with tests
./build.sh --debug --clean

# Run tests
cd build
ctest --output-on-failure

# Install development version
sudo cmake --install . --prefix /usr/local --component development
```

See [CONTRIBUTING.md](CONTRIBUTING.md) for more details.

---

**Questions?** Check the [FAQ](docs/user-manual/faq/index.html) or ask in [Discussions](https://github.com/R-Square-Innovative-Software/QuantilyxDoc/discussions).

---

*Built with ❤️ by R² Innovative Software*

*"Where innovation is the key to success"*