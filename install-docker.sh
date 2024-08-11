#!/bin/sh
scriptdir="$(dirname "$(realpath "$0")")"
targetdir="${BINDIR:-/usr/local/bin}"
cd "$scriptdir"/linux
make
mkdir -p "$targetdir"
cp bin/concord "$targetdir/concord"
