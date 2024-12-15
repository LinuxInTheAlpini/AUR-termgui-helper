# Nome del binario
TARGET = AUR

# File sorgente
SRC = AUR.c

# Compilatore
CC = gcc

# Opzioni di compilazione
CFLAGS = -Wall -O2

# Librerie necessarie
LIBS = -lcurl -lncurses

license=('MIT') # o 'GPL3', 'Apache', 'BSD'

# Obiettivo predefinito
all: $(TARGET)

# Compilazione del binario
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

# Pulizia
clean:
	rm -f $(TARGET)

# Installazione (opzionale)
install: $(TARGET)
	install -Dm755 $(TARGET) /usr/bin/$(TARGET)

# Disinstallazione (opzionale)
uninstall:
	rm -f /usr/bin/$(TARGET)

