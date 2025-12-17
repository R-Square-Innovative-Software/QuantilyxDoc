#!/bin/bash

###############################################################################
# QuantilyxDoc Build Script
# Copyright (C) 2025 R² Innovative Software
# "Where innovation is the key to success"
###############################################################################

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="build"
INSTALL_PREFIX="/usr/local"
BUILD_TYPE="Release"
JOBS=$(nproc)
ENABLE_TESTS="ON"
ENABLE_PLUGINS="ON"
ENABLE_WEB="ON"
ENABLE_TESSERACT="ON"
ENABLE_PADDLEOCR="OFF"
ENABLE_GPU="ON"
LEGACY_BUILD="OFF"

# Functions
print_header() {
    echo -e "${BLUE}"
    echo "╔══════════════════════════════════════════════════════════════╗"
    echo "║         QuantilyxDoc Build System v1.0                       ║"
    echo "║         R² Innovative Software                               ║"
    echo "║         \"Where innovation is the key to success\"             ║"
    echo "╚══════════════════════════════════════════════════════════════╝"
    echo -e "${NC}"
}

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Options:
    -h, --help              Show this help message
    -d, --debug             Build in Debug mode (default: Release)
    -c, --clean             Clean build directory before building
    -i, --install           Install after building
    -t, --no-tests          Disable building tests
    -p, --no-plugins        Disable building plugins
    -w, --no-web            Disable web interface
    -l, --legacy            Build for legacy systems (Debian 9)
    -j, --jobs N            Number of parallel jobs (default: $(nproc))
    --prefix PATH           Installation prefix (default: /usr/local)
    --no-tesseract          Disable Tesseract OCR
    --enable-paddleocr      Enable PaddleOCR support
    --no-gpu                Disable GPU acceleration

Examples:
    $0                      # Standard release build
    $0 --debug              # Debug build
    $0 --clean --install    # Clean, build, and install
    $0 --legacy             # Build for Debian 9
    $0 -j 8                 # Build with 8 parallel jobs

EOF
}

check_dependencies() {
    print_info "Checking dependencies..."
    
    local missing_deps=()
    
    # Check for required tools
    command -v cmake >/dev/null 2>&1 || missing_deps+=("cmake")
    command -v g++ >/dev/null 2>&1 || missing_deps+=("g++")
    command -v pkg-config >/dev/null 2>&1 || missing_deps+=("pkg-config")
    
    # Check for Qt5
    if ! pkg-config --exists Qt5Core; then
        missing_deps+=("qt5-default or qtbase5-dev")
    fi
    
    # Check for Poppler
    if ! pkg-config --exists poppler-qt5; then
        missing_deps+=("libpoppler-qt5-dev")
    fi
    
    # Check for OpenSSL
    if ! pkg-config --exists openssl; then
        missing_deps+=("libssl-dev")
    fi
    
    if [ ${#missing_deps[@]} -gt 0 ]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        print_info "Install with:"
        echo "    sudo apt-get install ${missing_deps[*]}"
        return 1
    fi
    
    print_success "All dependencies found"
    return 0
}

configure_build() {
    print_info "Configuring build..."
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    cmake .. \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
        -DBUILD_TESTS="$ENABLE_TESTS" \
        -DBUILD_PLUGINS="$ENABLE_PLUGINS" \
        -DBUILD_WEB_INTERFACE="$ENABLE_WEB" \
        -DENABLE_OCR_TESSERACT="$ENABLE_TESSERACT" \
        -DENABLE_OCR_PADDLEOCR="$ENABLE_PADDLEOCR" \
        -DENABLE_GPU_ACCELERATION="$ENABLE_GPU" \
        -DBUILD_LEGACY="$LEGACY_BUILD"
    
    cd ..
    
    print_success "Configuration complete"
}

build_project() {
    print_info "Building project with $JOBS parallel jobs..."
    
    cd "$BUILD_DIR"
    make -j"$JOBS"
    cd ..
    
    print_success "Build complete"
}

run_tests() {
    if [ "$ENABLE_TESTS" = "ON" ]; then
        print_info "Running tests..."
        cd "$BUILD_DIR"
        ctest --output-on-failure
        cd ..
        print_success "All tests passed"
    fi
}

install_project() {
    print_info "Installing QuantilyxDoc..."
    
    cd "$BUILD_DIR"
    
    if [ "$EUID" -ne 0 ]; then
        print_warning "Installation requires root privileges"
        sudo make install
    else
        make install
    fi
    
    cd ..
    
    # Update desktop database
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database ~/.local/share/applications/ 2>/dev/null || true
    fi
    
    print_success "Installation complete"
}

clean_build() {
    print_info "Cleaning build directory..."
    
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        print_success "Build directory cleaned"
    else
        print_info "Build directory doesn't exist, nothing to clean"
    fi
}

# Parse arguments
DO_CLEAN=false
DO_INSTALL=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            usage
            exit 0
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -c|--clean)
            DO_CLEAN=true
            shift
            ;;
        -i|--install)
            DO_INSTALL=true
            shift
            ;;
        -t|--no-tests)
            ENABLE_TESTS="OFF"
            shift
            ;;
        -p|--no-plugins)
            ENABLE_PLUGINS="OFF"
            shift
            ;;
        -w|--no-web)
            ENABLE_WEB="OFF"
            shift
            ;;
        -l|--legacy)
            LEGACY_BUILD="ON"
            shift
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        --prefix)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        --no-tesseract)
            ENABLE_TESSERACT="OFF"
            shift
            ;;
        --enable-paddleocr)
            ENABLE_PADDLEOCR="ON"
            shift
            ;;
        --no-gpu)
            ENABLE_GPU="OFF"
            shift
            ;;
        *)
            print_error "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Main execution
print_header

print_info "Build configuration:"
echo "  Build type: $BUILD_TYPE"
echo "  Install prefix: $INSTALL_PREFIX"
echo "  Parallel jobs: $JOBS"
echo "  Build tests: $ENABLE_TESTS"
echo "  Build plugins: $ENABLE_PLUGINS"
echo "  Build web interface: $ENABLE_WEB"
echo "  Tesseract OCR: $ENABLE_TESSERACT"
echo "  PaddleOCR: $ENABLE_PADDLEOCR"
echo "  GPU acceleration: $ENABLE_GPU"
echo "  Legacy build: $LEGACY_BUILD"
echo ""

# Check dependencies
if ! check_dependencies; then
    exit 1
fi

# Clean if requested
if [ "$DO_CLEAN" = true ]; then
    clean_build
fi

# Build process
configure_build
build_project
run_tests

# Install if requested
if [ "$DO_INSTALL" = true ]; then
    install_project
fi

print_success "Build process completed successfully!"
echo ""
print_info "To install, run:"
echo "    sudo ./build.sh --install"
echo ""
print_info "To run:"
echo "    $INSTALL_PREFIX/bin/quantilyxdoc"
echo ""
print_info "Documentation:"
echo "    file://$(pwd)/docs/user-manual/index.html"
echo ""