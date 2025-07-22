#!/bin/dash

: "${INSTALL_PATH:=$HOME/.local}"
DISTRO=$(awk -F'=' '/^ID=/ {print $2}' /etc/os-release)
DISTRO_PRETTY=$(awk -F'=' '/^PRETTY_NAME=/ { gsub(/"/, "", $2); print $2 }' /etc/os-release)

if [ -d "lsfg-vk" ]; then
  cd lsfg-vk
  git pull
else
  git clone https://github.com/soccera1/lsfg-vk
  cd lsfg-vk
fi

# offer to uninstall
echo -n "Do you want to uninstall lsfg-vk? (y/n) "
read -r uninstall_answer < /dev/tty
if [ "$uninstall_answer" = "y" ]; then
  rm -v $INSTALL_PATH/lib/liblsfg-vk.so
  rm -v $INSTALL_PATH/share/vulkan/implicit_layer.d/VkLayer_LS_frame_generation.json
  rm -v "$SHA_FILE"
  echo "Uninstallation completed."
fi
