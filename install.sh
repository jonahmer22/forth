#!/bin/sh

INSTALL_DIR="/usr/local/bin"
BINARY="forth"

# update submodules and stuff
git submodule init
git submodule update

# build first
make || exit 1

# install
echo "installing $BINARY to $INSTALL_DIR"
cp "$BINARY" "$INSTALL_DIR/$BINARY"
chmod 755 "$INSTALL_DIR/$BINARY"

echo "done"
