#!/bin/sh
cd "$(dirname "$0")"

dirbase="$(basename "$(pwd)")"

if [ -n "$MINI_VERSION_TAG" ]; then
    version="$MINI_VERSION_TAG"
elif [ -e ".git" ]; then
    version="$(git describe --tags --always --dirty)"
elif [ "$(echo "${dirbase}" | cut -c1-5)" = "MINI-" ]; then
    version=$(echo "${dirbase}" | cut -c6-)
    version="v${version##v}"
else
    version="unknown"
fi

echo "#define BUILD_TAG \"$version\""
