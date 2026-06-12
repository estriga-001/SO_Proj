/**
 * @file shared_memory.h
 * @brief Definições e protótipos para a memória partilhada do sistema.
 *
 * Contém as estruturas de dados partilhadas entre processos (sample_t
 * e shared_data_t) e os protótipos das funções de inicialização e
 * limpeza da memória partilhada POSIX.
 */

#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include "macros.h"

/* ============================================================
 * Estruturas de dados
 * ============================================================ */

/**
 * Representa uma amostra recolhida por um drone.
 */
typedef struct {
    int id;            /**< Identificador único da amostra */
    int drone_id;      /**< Identificador do drone que recolheu */
    time_t created_at; /**< Timestamp do momento de criação */
} sample_t;

/**
 * Estrutura principal de dados partilhados entre processos.
 *
 * Contém o tabuleiro (buffer circular), contadores, flags de estado
 * e mecanismos de sincronização (mutex + condition variables).
 * Toda esta estrutura reside em memória partilhada POSIX.
 */
typedef struct {
    /* --- Buffer circular (tabuleiro de amostras) --- */
    sample_t board[BOARD_CAPACITY]; /**< Tabuleiro de amostras */
    int count;                      /**< Número atual de amostras no tabuleiro */
    int in;                         /**< Índice de inserção (próxima posição livre) */
    int out;                        /**< Índice de remoção (próxima amostra a retirar) */

    /* --- Gerador de IDs --- */
    int next_sample_id;             /**< Próximo ID a atribuir a uma amostra */

    /* --- Estado do sistema --- */
    int analysis_active;            /**< 1 = análise ativa, 0 = análise em standby */
    int terminate;                  /**< 1 = sistema a terminar, 0 = em execução */

    /* --- Contadores estatísticos --- */
    int total_created;              /**< Total de amostras criadas */
    int total_deposited;            /**< Total de amostras depositadas no tabuleiro */
    int total_analyzed;             /**< Total de amostras analisadas e descartadas */
    int total_wait_full;            /**< Total de esperas por tabuleiro cheio */
    int total_wait_empty;           /**< Total de esperas por tabuleiro vazio */

    /* --- Sincronização por Semáforos (PTHREAD_PROCESS_SHARED) --- */
    sem_t sem_mutex;                /**< Semáforo para exclusão mútua no acesso ao tabuleiro (inicializado a 1) */
    sem_t sem_empty;                /**< Semáforo para contar slots livres no tabuleiro (inicializado a BOARD_CAPACITY) */
    sem_t sem_full;                 /**< Semáforo para contar amostras disponíveis no tabuleiro (inicializado a 0) */
    sem_t sem_analysis;             /**< Semáforo para suspender braços robóticos em standby/desativados (inicializado a 0) */
    int num_waiting_analysis;       /**< Contador de braços robóticos à espera no sem_analysis */
} shared_data_t;

/* ============================================================
 * Protótipos de funções
 * ============================================================ */

/**
 * Inicializa a memória partilhada do sistema.
 *
 * Cria a região de memória partilhada POSIX com shm_open,
 * define o tamanho com ftruncate, faz mmap e inicializa
 * todos os campos da estrutura shared_data_t, incluindo
 * mutex e condition variables com atributo PTHREAD_PROCESS_SHARED.
 *
 * @return Ponteiro para shared_data_t em caso de sucesso;
 *         NULL em caso de erro.
 *
 * Possíveis erros:
 * - shm_open falhar (permissões, nome inválido).
 * - ftruncate falhar (espaço insuficiente).
 * - mmap falhar (memória insuficiente).
 * - pthread_mutex_init / pthread_cond_init falharem.
 */
shared_data_t *init_shared_memory(void);

/**
 * Liberta e remove a memória partilhada do sistema.
 *
 * Destrói mutex e condition variables, faz munmap da região
 * mapeada e remove a memória partilhada com shm_unlink.
 *
 * @param shared Ponteiro para a estrutura de dados partilhados.
 *
 * Possíveis erros:
 * - pthread_mutex_destroy / pthread_cond_destroy falharem.
 * - munmap falhar.
 * - shm_unlink falhar.
 */
void cleanup_shared_memory(shared_data_t *shared);

/**
 * Abre a memória partilhada já existente (para processos filhos).
 *
 * Abre a região de memória partilhada POSIX já criada pelo
 * processo principal e faz mmap para obter acesso aos dados.
 *
 * @return Ponteiro para shared_data_t em caso de sucesso;
 *         NULL em caso de erro.
 */
shared_data_t *open_shared_memory(void);

#endif /* SHARED_MEMORY_H */
