#!/bin/bash

# Nome del file sorgente e del comando
SOURCE_FILE="AUR.c"
EXECUTABLE_NAME="aur"

# Funzione per verificare l'esito di un comando
check_status() {
    if [[ $? -ne 0 ]]; then
        echo "Error: $1 failed. Exiting."
        exit 1
    fi
}

# Assicurati di avere i permessi di root
if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root. Use sudo."
    exit 1
fi

# Aggiorna i pacchetti e installa le dipendenze
echo "Updating package list and installing dependencies..."
sudo pacman -Syu --noconfirm
check_status "Updating package list"

sudo pacman -S --noconfirm gcc curl ncurses base-devel git
check_status "Installing dependencies"

# Controllo se esiste il file sorgente
if [[ ! -f "$SOURCE_FILE" ]]; then
    echo "Error: Source file '$SOURCE_FILE' not found in the current directory."
    exit 1
fi

# Compila il programma
echo "Compiling $SOURCE_FILE..."
gcc "$SOURCE_FILE" -o "$EXECUTABLE_NAME" -lcurl -lncurses
check_status "Compilation"

# Copia l'eseguibile in /usr/local/bin
echo "Installing $EXECUTABLE_NAME to /usr/local/bin..."
cp "$EXECUTABLE_NAME" /usr/local/bin/
check_status "Copying executable"

# Imposta i permessi di esecuzione
chmod +x /usr/local/bin/"$EXECUTABLE_NAME"
check_status "Setting permissions"

# Verifica dell'installazione
if [[ -f "/usr/local/bin/$EXECUTABLE_NAME" ]]; then
    echo "Installation complete! You can now run the command using '$EXECUTABLE_NAME'."
else
    echo "Error: Installation failed."
    exit 1
fi

