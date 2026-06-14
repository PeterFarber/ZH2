#!/usr/bin/env bash
# One-time Linux setup: install system build dependencies and bootstrap vcpkg.
set -euo pipefail

echo "== HockeyGame Linux setup =="

install_apt() {
    sudo apt update
    sudo apt install -y \
        build-essential cmake ninja-build git pkg-config \
        zip unzip tar curl \
        libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev \
        libxinerama-dev libwayland-dev libxkbcommon-dev wayland-protocols
}

install_pacman() {
    sudo pacman -S --needed --noconfirm \
        base-devel cmake ninja git pkgconf zip unzip tar curl \
        libx11 libxext libxrandr libxcursor libxi libxinerama \
        wayland wayland-protocols libxkbcommon mesa autoconf-archive
}

install_dnf() {
    sudo dnf install -y \
        gcc-c++ cmake ninja-build git pkgconf zip unzip tar curl \
        libX11-devel libXext-devel libXrandr-devel libXcursor-devel libXi-devel \
        libXinerama-devel wayland-devel libxkbcommon-devel wayland-protocols-devel
}

if command -v apt >/dev/null 2>&1; then
    install_apt
elif command -v pacman >/dev/null 2>&1; then
    install_pacman
elif command -v dnf >/dev/null 2>&1; then
    install_dnf
else
    echo "Unknown package manager. Install a C++20 toolchain, CMake, Ninja, git," \
         "and SDL3 X11/Wayland build dependencies manually." >&2
fi

# Bootstrap vcpkg if VCPKG_ROOT points at a checkout that is not yet bootstrapped.
if [[ -n "${VCPKG_ROOT:-}" ]]; then
    if [[ -x "${VCPKG_ROOT}/vcpkg" ]]; then
        echo "vcpkg already bootstrapped at ${VCPKG_ROOT}."
    elif [[ -f "${VCPKG_ROOT}/bootstrap-vcpkg.sh" ]]; then
        echo "Bootstrapping vcpkg at ${VCPKG_ROOT}..."
        "${VCPKG_ROOT}/bootstrap-vcpkg.sh" -disableMetrics
    else
        echo "VCPKG_ROOT is set to '${VCPKG_ROOT}' but no vcpkg checkout was found there." >&2
    fi
else
    echo "VCPKG_ROOT is not set. Clone vcpkg and export VCPKG_ROOT, for example:" >&2
    echo "  git clone https://github.com/microsoft/vcpkg \"\$HOME/vcpkg\"" >&2
    echo "  \"\$HOME/vcpkg/bootstrap-vcpkg.sh\" -disableMetrics" >&2
    echo "  export VCPKG_ROOT=\"\$HOME/vcpkg\"" >&2
fi

echo "== Setup complete =="
