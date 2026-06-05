#!/bin/sh

INSTALL_DIR="/usr/local/bin"
BINARY="forth"

if [ ! -f "$INSTALL_DIR/$BINARY" ]; then
    echo "$BINARY is not installed"
    exit 0
fi

echo "removing $INSTALL_DIR/$BINARY"
rm "$INSTALL_DIR/$BINARY"

echo "done"
