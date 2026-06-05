#!/bin/sh

INSTALL_DIR="/usr/local/bin"
BINARY="forth"

# build first
make || exit 1

# install
echo "installing $BINARY to $INSTALL_DIR"
cp "$BINARY" "$INSTALL_DIR/$BINARY"
chmod 755 "$INSTALL_DIR/$BINARY"

echo "done"
