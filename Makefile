# ==============================================================================
# Makefile — Estação Autónoma de Exploração Planetária
# Projeto de Sistemas Operativos 2025/2026
# ==============================================================================
#
# Utilização:
#   make        — Compila o projeto
#   make all    — Idem
#   make run    — Compila e executa
#   make clean  — Remove ficheiros gerados (build/ e bin/)
#   make zip    — Cria ZIP para entrega em entrega/
#
# ==============================================================================

# Compilador e flags
CC       = gcc
CFLAGS   = -Wall -Wextra -Werror -pedantic -std=c11 -D_XOPEN_SOURCE=700 -pthread
LDLIBS   = -pthread -lrt

# Diretórios
SRC_DIR   = src
INC_DIR   = src/include
BUILD_DIR = build
BIN_DIR   = bin

# Ficheiros fonte e objetos
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Executável final
TARGET = $(BIN_DIR)/estacao

# ==============================================================================
# Regras
# ==============================================================================

.PHONY: all clean run zip

## Regra principal: compila o executável
all: $(TARGET)

## Linkagem final
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

## Compilação de cada ficheiro .c para .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c -o $@ $<

## Criar diretório build/ se não existir
$(BUILD_DIR):
	mkdir -p $@

## Criar diretório bin/ se não existir
$(BIN_DIR):
	mkdir -p $@

## Executar o programa
run: $(TARGET)
	./$(TARGET)

## Limpar ficheiros gerados
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

## Criar ZIP para entrega
zip: clean
	mkdir -p entrega
	zip -r entrega/projeto_so_fase2.zip \
		src/ \
		docs/ \
		subjects/ \
		Makefile \
		README.md \
		-x "*.o" "*/.*"
