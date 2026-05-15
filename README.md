# Estação Autónoma de Exploração Planetária

**Projeto de Sistemas Operativos — 2025/2026**

## Descrição

Aplicação em C que simula uma estação autónoma de exploração planetária em Marte. A estação possui uma frota de drones exploratórios que recolhem amostras do solo, um tabuleiro de armazenamento temporário e um sistema de análise científica com braços robóticos que analisam e descartam as amostras.

## Requisitos

- **Sistema Operativo**: Linux / WSL Ubuntu
- **Compilador**: GCC com suporte a C11 e POSIX threads
- **Bibliotecas**: pthread, rt (POSIX shared memory)

## Compilação

```bash
make
```

O executável é gerado em `bin/estacao`.

## Execução

```bash
make run
# ou diretamente:
./bin/estacao
```

## Controlo do Sistema

| Tecla | Ação |
|-------|------|
| `Ctrl-Z` | Ativa/desativa o sistema de análise (toggle) |
| `Ctrl-C` | Termina o sistema de forma limpa |

### Quando a análise está desativada:
- Os drones continuam a recolher e depositar amostras.
- Os braços robóticos ficam em standby.
- Se o tabuleiro encher, os drones ficam bloqueados à espera de espaço.

### Quando a análise é reativada:
- Os braços robóticos retomam a análise das amostras disponíveis.
- Os drones bloqueados são desbloqueados (se aplicável).

## Limpar ficheiros gerados

```bash
make clean
```

Remove as pastas `build/` e `bin/` com todos os objetos e executáveis.

## Criar ZIP para entrega

```bash
make zip
```

Gera `entrega/projeto_so_fase2.zip` contendo código fonte, documentação e Makefile.

## Arquitetura

```
Processo Principal (main.c)
├── fork() → Processo de Exploração (exploration.c)
│             ├── Thread Drone 1 (drone.c)
│             ├── Thread Drone 2
│             └── Thread Drone 3
│
├── fork() → Processo de Análise (analysis.c)
│             ├── Thread Analisador 1 (robotic_arm.c)
│             └── Thread Analisador 2
│
└── Memória Partilhada (shared_memory.c)
              ├── Tabuleiro [10 posições]
              ├── Mutex (PTHREAD_PROCESS_SHARED)
              ├── Cond: can_deposit
              └── Cond: can_analyze
```

### Sincronização
- **pthread_mutex_t** com `PTHREAD_PROCESS_SHARED` para acesso exclusivo ao tabuleiro.
- **pthread_cond_t** com `PTHREAD_PROCESS_SHARED`:
  - `can_deposit`: drones esperam quando o tabuleiro está cheio.
  - `can_analyze`: braços esperam quando o tabuleiro está vazio ou análise desativada.
- **Sem espera ativa**: todas as esperas usam `pthread_cond_wait`.

## Ficheiros Principais

```
src/
├── main.c              — Processo principal
├── shared_memory.c     — Memória partilhada POSIX
├── exploration.c       — Módulo de exploração (processo)
├── analysis.c          — Módulo de análise (processo)
├── drone.c             — Thread do drone
├── robotic_arm.c       — Thread do braço robótico
├── signals.c           — Tratamento de sinais
├── logger.c            — Logging centralizado
└── include/
    ├── macros.h         — Constantes do projeto
    ├── projeto.h        — Header principal
    ├── shared_memory.h  — Estruturas e protótipos SHM
    ├── exploration.h    — Protótipo do módulo exploração
    ├── analysis.h       — Protótipo do módulo análise
    ├── drone.h          — Protótipo e args do drone
    ├── robotic_arm.h    — Protótipo e args do analisador
    ├── signals.h        — Protótipos e flags de sinais
    └── logger.h         — Protótipos de logging
```

## Documentação

- [Plano de Implementação](docs/plano_implementacao.md)
- [Arquitetura](docs/arquitetura.md)
- [Testes](docs/testes.md)
- [Modelo Fase 2 Preenchido](docs/modelo_fase2_preenchido.txt)
