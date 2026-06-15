#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")"; pwd)
FALLBACK_WRAPPER="$SCRIPT_DIR/../../../../../OSAIG/android/gradlew"

if [ -z "${ANDROID_HOME:-}" ] && [ -d /usr/lib/android-sdk ]; then
    export ANDROID_HOME=/usr/lib/android-sdk
fi

if command -v gradle >/dev/null 2>&1; then
    exec gradle -p "$SCRIPT_DIR" assembleDebug
fi

if [ -x "$FALLBACK_WRAPPER" ]; then
    exec "$FALLBACK_WRAPPER" -p "$SCRIPT_DIR" assembleDebug
fi

echo "ERROR: Gradle is not installed."
echo "Install Gradle, open this directory with Android Studio, or provide a Gradle wrapper."
exit 1
