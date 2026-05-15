# Arquitetura do Sistema

## Diagrama Geral

```
┌─────────────────────────────────────────────────────────────────┐
│                      PROCESSO PRINCIPAL                         │
│                          main.c                                 │
│                                                                 │
│  1. init_shared_memory()  → Cria SHM + mutex + condvars        │
│  2. install_signal_handlers() → SIGINT + SIGTSTP                │
│  3. fork() ─────────────────────┐                               │
│  4. fork() ───────────────┐     │                               │
│  5. Loop: pause()         │     │                               │
│     • SIGTSTP → toggle    │     │                               │
│     • SIGINT  → terminar  │     │                               │
│  6. waitpid() x2          │     │                               │
│  7. cleanup_shared_memory()│     │                               │
│                            │     │                               │
└────────────────────────────┼─────┼───────────────────────────────┘
                             │     │
              ┌──────────────┘     └──────────────┐
              ▼                                   ▼
┌──────────────────────────┐   ┌──────────────────────────────┐
│  PROCESSO DE EXPLORAÇÃO  │   │  PROCESSO DE ANÁLISE         │
│     exploration.c        │   │     analysis.c               │
│                          │   │                              │
│  pthread_create x3       │   │  pthread_create x2           │
│  ┌────────────────────┐  │   │  ┌────────────────────────┐  │
│  │ DRONE 1   (drone.c)│  │   │  │ ANALISADOR 1           │  │
│  │ DRONE 2   (drone.c)│  │   │  │ (robotic_arm.c)        │  │
│  │ DRONE 3   (drone.c)│  │   │  │ ANALISADOR 2           │  │
│  └────────────────────┘  │   │  │ (robotic_arm.c)        │  │
│  pthread_join x3         │   │  └────────────────────────┘  │
└──────────────────────────┘   │  pthread_join x2             │
                               └──────────────────────────────┘

        ╔═══════════════════════════════════════════════╗
        ║          MEMÓRIA PARTILHADA POSIX             ║
        ║              (shared_memory.c)                ║
        ║                                               ║
        ║  ┌─────────────────────────────────────────┐  ║
        ║  │   TABULEIRO (buffer circular, 10 pos.)  │  ║
        ║  │   [0] [1] [2] [3] [4] [5] [6] [7] [8] [9]║
        ║  │    ↑ out                  in ↑          │  ║
        ║  └─────────────────────────────────────────┘  ║
        ║                                               ║
        ║  count, next_sample_id                        ║
        ║  analysis_active, terminate                   ║
        ║  Contadores estatísticos                      ║
        ║                                               ║
        ║  pthread_mutex_t  mutex     (PROCESS_SHARED)  ║
        ║  pthread_cond_t   can_deposit                 ║
        ║  pthread_cond_t   can_analyze                 ║
        ╚═══════════════════════════════════════════════╝
```

## Processos

### Processo Principal (`main.c`)
- **PID**: processo pai.
- **Responsabilidades**: inicialização, gestão de sinais, criação de filhos, terminação.
- **Sinais tratados**:
  - `SIGINT` (Ctrl-C): terminação limpa.
  - `SIGTSTP` (Ctrl-Z): toggle de análise.

### Processo de Exploração (`exploration.c`)
- **PID**: processo filho criado com `fork()`.
- **Responsabilidades**: criar e gerir 3 threads de drones.
- **Terminação**: quando `shared->terminate == TRUE`, as threads verificam a flag e retornam.

### Processo de Análise (`analysis.c`)
- **PID**: processo filho criado com `fork()`.
- **Responsabilidades**: criar e gerir 2 threads de braços robóticos.
- **Terminação**: idem ao processo de exploração.

## Threads

### Drones (`drone.c`)
- **Quantidade**: 3 (NUM_DRONES).
- **Ciclo**: recolhe amostra → espera espaço → deposita → repete.
- **Tempo de recolha**: 5 segundos (DRONE_DELIVERY_TIME).

### Braços Robóticos (`robotic_arm.c`)
- **Quantidade**: 2 (NUM_ANALYZERS).
- **Ciclo**: espera amostra + análise ativa → retira → analisa (1-3s) → descarta → repete.
- **Análise fora da secção crítica**: o sleep de análise é feito após libertar o mutex.

## Fluxo de Amostras

```
                  PRODUÇÃO                    CONSUMO
              ┌──────────────┐          ┌────────────────────┐
              │              │          │                    │
    ┌─────────▼──────────┐   │   ┌──────▼───────────┐       │
    │ 1. Drone recolhe   │   │   │ 5. Braço espera  │       │
    │    amostra (5s)    │   │   │    amostra E     │       │
    └─────────┬──────────┘   │   │    análise ativa │       │
              │              │   └──────┬───────────┘       │
    ┌─────────▼──────────┐   │   ┌──────▼───────────┐       │
    │ 2. Drone espera    │   │   │ 6. Braço retira  │       │
    │    espaço          │   │   │    amostra       │       │
    │    (cond_wait se   │   │   │    (lock mutex)  │       │
    │     cheio)         │   │   └──────┬───────────┘       │
    └─────────┬──────────┘   │          │                    │
              │              │   ┌──────▼───────────┐       │
    ┌─────────▼──────────┐   │   │ 7. Braço analisa │       │
    │ 3. Drone deposita  │   │   │    (1-3s, fora   │       │
    │    amostra         │   │   │     do mutex)    │       │
    │    (lock mutex)    │   │   └──────┬───────────┘       │
    └─────────┬──────────┘   │          │                    │
              │              │   ┌──────▼───────────┐       │
    ┌─────────▼──────────┐   │   │ 8. Braço         │       │
    │ 4. Broadcast       │   │   │    descarta      │       │
    │    can_analyze     │───┘   │    amostra       │       │
    └────────────────────┘       └──────┬───────────┘       │
                                        │                    │
                                 ┌──────▼───────────┐       │
                                 │ 9. Broadcast     │       │
                                 │    can_deposit   │───────┘
                                 └──────────────────┘
```

## Sincronização Detalhada

### Secção Crítica (protegida por mutex)
- Leitura/escrita de `board[]`, `count`, `in`, `out`.
- Leitura/escrita de `next_sample_id`.
- Leitura/escrita de contadores estatísticos.
- Leitura/escrita de `analysis_active` (pelo processo principal).
- Leitura/escrita de `terminate`.

### Condition Variables
| Variável | Quem espera | Quem sinaliza | Condição de espera |
|----------|-------------|---------------|-------------------|
| `can_deposit` | Drones | Braços robóticos | `count == BOARD_CAPACITY` |
| `can_analyze` | Braços robóticos | Drones + Main | `count == 0` ou `!analysis_active` |

### Invariantes
1. `0 <= count <= BOARD_CAPACITY` (sempre).
2. `in` e `out` são sempre válidos: `0 <= in,out < BOARD_CAPACITY`.
3. O mutex é sempre libertado antes de qualquer `sleep()`.
4. Todas as threads verificam `terminate` após acordar de `cond_wait`.
