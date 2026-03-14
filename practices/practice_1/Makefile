# Nombre del compilador
CC = gcc

# Opciones de compilación

# Archivos fuente
SRC_INTERFAZ = ui.c
SRC_BUSCADOR =hash_search.c
SRC_HASH = hash.c

# Ejecutables
BIN_INTERFAZ = interfaz
BIN_BUSCADOR = buscador

# Regla por defecto: compilar ambos
all: $(BIN_INTERFAZ) $(BIN_BUSCADOR)

# Compilar interfaz
$(BIN_INTERFAZ): $(SRC_INTERFAZ)
	$(CC) $(CFLAGS) $(SRC_INTERFAZ) -o $(BIN_INTERFAZ)

# Compilar buscador
$(BIN_BUSCADOR): $(SRC_BUSCADOR)
	$(CC) $(CFLAGS) $(SRC_BUSCADOR) $(SRC_HASH) -o $(BIN_BUSCADOR)

# Limpiar binarios
clean:
	rm -f $(BIN_INTERFAZ) $(BIN_BUSCADOR)
