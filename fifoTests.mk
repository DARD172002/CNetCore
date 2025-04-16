
# Variables
CLIENTE_BIN = client/client  # Ruta al ejecutable ya compilado
SERVER_PORT = 8080       # Puerto del servidor
DOWNLOAD_DIR = downloads  # Ruta de la carpeta para los archivos descargados

# Archivos de prueba
Prueba1 = index.html
Prueba2 = largefile.bin Hola.txt
Prueba3 = Hola.txt
Prueba4 = prueba.jpg 

# Agrupar todos
TODOS = $(Prueba1) $(Prueba2) $(Prueba3) $(Prueba4)

# Crear carpeta de descargas
prepare:
	@mkdir -p $(DOWNLOAD_DIR)

# Pruebas individuales
test1: prepare
	@./$(CLIENTE_BIN) $(SERVER_PORT) $(Prueba1)

test2: prepare
	@./$(CLIENTE_BIN) $(SERVER_PORT) $(Prueba2)

test3: prepare
	@./$(CLIENTE_BIN) $(SERVER_PORT) $(Prueba3)

test4: prepare
	@./$(CLIENTE_BIN) $(SERVER_PORT) $(Prueba4)

# Prueba todas a la vez (en paralelo)
testAll-fifo: prepare
	@$(MAKE) -f fifoTests.mk -j 4 test1 test2 test3 test4

# Limpiar
clean:
	rm -rf $(DOWNLOAD_DIR)
