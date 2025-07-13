#!/bin/sh

: "${INSTALL_PATH:=$HOME/.local}"
BASE_URL='https://pancake.gay/lsfg-vk'

# prompt for distro
echo "Which version would you like to install?"
echo "1) Arch Linux (Artix Linux, CachyOS, Steam Deck, etc.)"
echo "2) Debian"
echo "3) Ubuntu"
echo "4) Fedora"
printf "Enter the number (1-4): "
read -r version_choice < /dev/tty

case "$version_choice" in
    1) DISTRO="archlinux"; DISTRO_PRETTY="Arch Linux" ;;
    2) DISTRO="debian"; DISTRO_PRETTY="Debian" ;;
    3) DISTRO="ubuntu"; DISTRO_PRETTY="Ubuntu" ;;
    4) DISTRO="fedora"; DISTRO_PRETTY="Fedora" ;;
    *) echo "Invalid choice."; exit 1 ;;
esac

ZIP_NAME="lsfg-vk_${DISTRO}.zip"
SHA_NAME="lsfg-vk_${DISTRO}.zip.sha"
SHA_FILE="$INSTALL_PATH/share/lsfg-vk.sha"

# get local and remote versions
REMOTE_HASH=$(curl -fsSL "$BASE_URL/$SHA_NAME")
LOCAL_HASH=$(test -f "$SHA_FILE" && cat "$SHA_FILE")

if [ "$REMOTE_HASH" != "$LOCAL_HASH" ]; then
    # prompt user for confirmation
    echo -n "Are you sure you want to install lsfg-vk for ${DISTRO_PRETTY}? (y/n) "
    read -r answer < /dev/tty

    if [ "$answer" != "y" ]; then
        echo "Installation aborted."
        exit 0
    fi

    # download lsfg-vk
    curl -fsSL -o "/tmp/$ZIP_NAME" "$BASE_URL/$ZIP_NAME"
    if [ $? -ne 0 ]; then
        echo "Failed to download lsfg-vk. Please check your internet connection."
        exit 1
    fi

    # install lsfg-vk
    cd "$INSTALL_PATH" || exit 1
    unzip -o "/tmp/$ZIP_NAME"
    echo "$REMOTE_HASH" > "$SHA_FILE"
    rm -v "/tmp/$ZIP_NAME"

    echo "lsfg-vk for ${DISTRO_PRETTY} has been installed."
else
    echo "lsfg-vk is up to date."

    # offer to uninstall
    echo -n "Do you want to uninstall lsfg-vk? (y/n) "
    read -r uninstall_answer < /dev/tty
    if [ "$uninstall_answer" = "y" ]; then
        rm -v ~/.local/lib/liblsfg-vk.so
        rm -v ~/.local/share/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json
        rm -v "$SHA_FILE"
        echo "Uninstallation completed."
    fi
fi
