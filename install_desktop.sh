#!/bin/bash
# Desktop Integration Script for WhatSie (Qt5 version)

EXEC_PATH=$(realpath src/whatsie)
ICON_PATH=$(realpath dist/linux/hicolor/128x128/apps/com.ktechpit.whatsie.png)

mkdir -p ~/.local/share/applications ~/.local/share/icons/hicolor/128x128/apps ~/.config/autostart

# Copy Icon
cp "$ICON_PATH" ~/.local/share/icons/hicolor/128x128/apps/com.ktechpit.whatsie.png

# Create Desktop Entry
cat <<EOF > ~/.local/share/applications/whatsie.desktop
[Desktop Entry]
Name=WhatSie
GenericName=Qt WhatsApp Web Client
Comment=A WhatsApp Web Client using the QT framework
Icon=com.ktechpit.whatsie
Exec=$EXEC_PATH %u
Terminal=false
Type=Application
Categories=Chat;Network;InstantMessaging;Qt;
Keywords=WhatSie;WhatsApp;
StartupWMClass=whatsie
StartupNotify=true
MimeType=x-scheme-handler/whatsapp;
X-GNOME-UsesNotifications=true

Actions=Chat;Settings;Theme;Lock;About;

[Desktop Action Chat]
Name=New Chat
Exec=$EXEC_PATH -n

[Desktop Action Theme]
Name=Toggle theme
Exec=$EXEC_PATH -t

[Desktop Action Settings]
Name=Settings
Exec=$EXEC_PATH -s

[Desktop Action Lock]
Name=Lock
Exec=$EXEC_PATH -l

[Desktop Action About]
Name=About
Exec=$EXEC_PATH -i
EOF

chmod +x ~/.local/share/applications/whatsie.desktop

# Create Autostart Entry
cp ~/.local/share/applications/whatsie.desktop ~/.config/autostart/whatsie.desktop
echo "X-GNOME-Autostart-enabled=true" >> ~/.config/autostart/whatsie.desktop

echo "WhatSie has been integrated into your desktop environment."
echo "You can now find it in your application menu and it will start automatically on login."
