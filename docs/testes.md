# Testes Manuais

## 1. Compilação

### Teste 1.1: Compilação sem erros
```bash
make clean && make
```
**Resultado esperado**: Compilação completa sem erros nem warnings.

### Teste 1.2: Compilação com flags rigorosas
**Verificar**: O Makefile usa `-Wall -Wextra -Werror -pedantic -std=c11`.
**Resultado esperado**: Zero warnings (senão não compila com `-Werror`).

---

## 2. Execução Normal

### Teste 2.1: Inicialização do sistema
```bash
make run
```
**Resultado esperado**:
- Mensagem de inicialização do sistema.
- Shared memory criada.
- Handlers de sinais instalados.
- Processos de exploração e análise criados.
- Drones e analisadores iniciados.

### Teste 2.2: Drones depositam a cada 5 segundos
**Verificar**: As mensagens dos drones aparecem com intervalos de ~5 segundos.
**Resultado esperado**: Cada drone deposita uma amostra a cada 5 segundos.

### Teste 2.3: Análise demora entre 1 e 3 segundos
**Verificar**: As mensagens dos analisadores indicam tempos entre 1 e 3 segundos.
**Resultado esperado**: O tempo de análise varia aleatoriamente entre 1 e 3 segundos.

### Teste 2.4: Ocupação do tabuleiro correta
**Verificar**: As mensagens mostram a ocupação (X/10) e esta é consistente.
**Resultado esperado**: A ocupação nunca excede 10/10 e nunca é negativa.

---

## 3. Toggle da Análise (Ctrl-Z)

### Teste 3.1: Desativar análise
**Ação**: Pressionar `Ctrl-Z` durante a execução.
**Resultado esperado**:
- Mensagem: "SIGTSTP recebido. Sistema de analise DESATIVADO."
- Analisadores exibem: "Sistema de analise em standby."
- Drones continuam a depositar amostras.

### Teste 3.2: Reativar análise
**Ação**: Pressionar `Ctrl-Z` novamente.
**Resultado esperado**:
- Mensagem: "SIGTSTP recebido. Sistema de analise ATIVADO."
- Analisadores retomam a retirada e análise de amostras.

### Teste 3.3: Tabuleiro enche com análise desativada
**Ação**:
1. Pressionar `Ctrl-Z` para desativar análise.
2. Esperar até o tabuleiro encher (10/10).
**Resultado esperado**:
- Drones bloqueiam com mensagem: "Tabuleiro cheio. A aguardar espaco..."
- Ocupação mantém-se em 10/10.

### Teste 3.4: Drones desbloqueiam ao reativar
**Ação**: Após teste 3.3, pressionar `Ctrl-Z` para reativar.
**Resultado esperado**:
- Analisadores começam a retirar amostras.
- Drones são desbloqueados e voltam a depositar.

---

## 4. Terminação (Ctrl-C)

### Teste 4.1: Terminação limpa
**Ação**: Pressionar `Ctrl-C` durante a execução.
**Resultado esperado**:
- Mensagem: "SIGINT recebido. A terminar sistema..."
- Drones e analisadores imprimem "terminado."
- Estatísticas finais apresentadas.
- Mensagem: "Recursos libertados. Fim."

### Teste 4.2: Sem processos zombie
**Ação**: Após `Ctrl-C`, verificar:
```bash
ps aux | grep estacao
```
**Resultado esperado**: Nenhum processo `estacao` visível.

### Teste 4.3: Sem shared memory pendente
**Ação**: Após `Ctrl-C`, verificar:
```bash
ls /dev/shm/ | grep estacao
```
**Resultado esperado**: Nenhum ficheiro `estacao_planetaria_shm`.

### Teste 4.4: Terminação com análise desativada
**Ação**:
1. Pressionar `Ctrl-Z` para desativar análise.
2. Pressionar `Ctrl-C` para terminar.
**Resultado esperado**: Terminação limpa, sem bloqueios.

### Teste 4.5: Terminação com tabuleiro cheio
**Ação**:
1. Desativar análise e esperar tabuleiro encher.
2. Pressionar `Ctrl-C`.
**Resultado esperado**: Todos os drones bloqueados são desbloqueados e terminam.

---

## 5. Make Clean e Make Zip

### Teste 5.1: make clean
```bash
make clean
ls build/ bin/
```
**Resultado esperado**: Diretórios `build/` e `bin/` removidos.

### Teste 5.2: make zip
```bash
make zip
ls entrega/
```
**Resultado esperado**: Ficheiro `entrega/projeto_so_fase2.zip` criado.

### Teste 5.3: Conteúdo do ZIP
```bash
unzip -l entrega/projeto_so_fase2.zip
```
**Resultado esperado**: Contém `src/`, `docs/`, `subjects/`, `Makefile`, `README.md`.
Não contém `.o` nem executáveis.

---

## 6. Robustez

### Teste 6.1: Múltiplos Ctrl-Z seguidos
**Ação**: Pressionar `Ctrl-Z` várias vezes rapidamente.
**Resultado esperado**: Estado alterna corretamente sem crashes.

### Teste 6.2: Ctrl-C durante Ctrl-Z
**Ação**: Pressionar `Ctrl-Z` seguido imediatamente de `Ctrl-C`.
**Resultado esperado**: Terminação limpa.

### Teste 6.3: Execução prolongada
**Ação**: Deixar o programa correr por vários minutos.
**Resultado esperado**: Sem memory leaks, sem crashes, output consistente.
