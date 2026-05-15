# Plano de Implementação

## 1. Objetivo do Projeto

Desenvolver uma aplicação em C que simula uma estação autónoma de exploração planetária em Marte. A estação possui uma frota de drones exploratórios que recolhem amostras do solo e um sistema de análise científica com braços robóticos que analisam e descartam as amostras.

## 2. Interpretação do Enunciado

O sistema deve ter:
- **3 processos**: principal, exploração e análise científica.
- **5 threads**: 3 drones (no processo de exploração) e 2 braços robóticos (no processo de análise).
- **Memória partilhada POSIX**: tabuleiro de 10 posições partilhado entre processos.
- **Sincronização sem espera ativa**: mutex + condition variables com `PTHREAD_PROCESS_SHARED`.
- **Sinais**: SIGINT para terminar, SIGTSTP para toggle da análise.

## 3. Entidades Identificadas

| Entidade | Implementação | Justificação |
|----------|--------------|--------------|
| Módulo Principal | Processo principal | Gere todo o sistema, cria filhos, trata sinais |
| Módulo de Exploração | Processo filho (fork) | Separação de responsabilidades entre módulos |
| Drones (×3) | Threads no proc. exploração | Partilham memória do processo, mais eficiente |
| Módulo de Análise | Processo filho (fork) | Separação de responsabilidades entre módulos |
| Braços robóticos (×2) | Threads no proc. análise | Partilham memória do processo, mais eficiente |

## 4. Decisão Processo/Thread

- **Processos** para os módulos principais (exploração e análise) — isolamento e robustez.
- **Threads** para drones e braços robóticos — partilha eficiente de memória dentro do mesmo processo.
- A comunicação entre processos é feita via **memória partilhada POSIX** (`shm_open` + `mmap`).

## 5. Estrutura da Memória Partilhada

```c
typedef struct {
    sample_t board[10];        // Tabuleiro (buffer circular)
    int count, in, out;        // Gestão do buffer
    int next_sample_id;        // Gerador de IDs
    int analysis_active;       // Estado da análise
    int terminate;             // Flag de terminação
    int total_created, total_deposited, total_analyzed;  // Estatísticas
    int total_wait_full, total_wait_empty;               // Estatísticas
    pthread_mutex_t mutex;     // Mutex partilhado
    pthread_cond_t can_deposit;  // Condição: espaço livre
    pthread_cond_t can_analyze;  // Condição: amostra + análise ativa
} shared_data_t;
```

## 6. Mecanismos de Sincronização

- **pthread_mutex_t** com `PTHREAD_PROCESS_SHARED` — secção crítica para acesso ao tabuleiro.
- **pthread_cond_t** com `PTHREAD_PROCESS_SHARED`:
  - `can_deposit`: drones esperam quando tabuleiro cheio.
  - `can_analyze`: braços esperam quando tabuleiro vazio ou análise desativada.
- Sem espera ativa — todas as esperas usam `pthread_cond_wait`.

## 7. Estratégia para SIGINT

1. Handler seta flag `volatile sig_atomic_t got_sigint = 1`.
2. Processo principal deteta a flag no loop (após `pause()` retornar).
3. Seta `shared->terminate = TRUE`.
4. Faz `pthread_cond_broadcast` em ambas as condition variables.
5. Envia `SIGINT` aos processos filhos.
6. Espera pelos filhos com `waitpid`.
7. Liberta memória partilhada (destroy mutex/conds, munmap, shm_unlink).

## 8. Estratégia para SIGTSTP

1. Handler seta flag `volatile sig_atomic_t got_sigtstp = 1`.
2. Processo principal deteta a flag no loop.
3. Faz toggle de `shared->analysis_active`.
4. Se reativado, faz `pthread_cond_broadcast(&shared->can_analyze)`.

## 9. Riscos Previstos

| Risco | Mitigação |
|-------|-----------|
| Deadlock | Usar sempre lock/unlock na mesma ordem; análise fora da secção crítica |
| Condições de corrida | Todas as alterações ao tabuleiro protegidas por mutex |
| Processos zombie | waitpid por todos os filhos antes de terminar |
| SHM pendente | shm_unlink no cleanup, mesmo em caso de erro |
| Threads bloqueadas na terminação | broadcast em todas as condvars + envio de SIGINT |

## 10. Plano de Tarefas

1. Criar estrutura de diretórios e headers.
2. Implementar memória partilhada.
3. Implementar logger.
4. Implementar tratamento de sinais.
5. Implementar processo principal.
6. Implementar módulo de exploração + drones.
7. Implementar módulo de análise + braços robóticos.
8. Testes e depuração.
9. Documentação e ZIP.

## 11. Possíveis Melhorias Futuras

- Logging para ficheiro em vez de stdout.
- Configuração dinâmica via ficheiro de configuração.
- Interface de texto interativa.
- Estatísticas em tempo real.
- Tipos de amostras diferentes com prioridades.
