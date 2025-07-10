#!/bin/sh

: "${INSTALL_PATH:=$HOME/.local}"
BASE_URL='https://pancake.gay/lsfg-vk'

# get local and remote versions
REMOTE_HASH=$(curl -fsSL "$BASE_URL/lsfg-vk.zip.sha")
LOCAL_HASH=$(test -f "$INSTALL_PATH/share/lsfg-vk.sha" && cat "$INSTALL_PATH/share/lsfg-vk.sha")

if [ "$REMOTE_HASH" != "$LOCAL_HASH" ]; then
    # prompt user for confirmation
    echo -n "Do you wish to install the latest version of lsfg-vk? (y/n) "
    read -r answer < /dev/tty

    if [ "$answer" != "y" ]; then
        echo "Installation aborted."
        exit 0
    fi

    # download lsfg-vk
    curl -fsSL -o "/tmp/lsfg-vk.zip" "$BASE_URL/lsfg-vk.zip"
    if [ $? -ne 0 ]; then
        echo "Failed to download lsfg-vk. Please check your internet connection."
        exit 1
    fi

    # install lsfg-vk
    cd "$INSTALL_PATH" || exit 1
    unzip -oqq "/tmp/lsfg-vk.zip"
    echo "$REMOTE_HASH" > share/lsfg-vk.sha

    echo "lsfg-vk has been installed."
else
    echo "lsfg-vk is already up to date."
fi
