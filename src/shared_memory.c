/**
 * @file shared_memory.c
 * @brief Implementação da memória partilhada POSIX do sistema.
 *
 * Responsável por criar, inicializar, abrir e destruir a região
 * de memória partilhada usada para comunicação entre o processo
 * principal, o processo de exploração e o processo de análise.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "include/shared_memory.h"
#include "include/logger.h"

/**
 * Inicializa a memória partilhada do sistema.
 *
 * Cria a região de memória partilhada POSIX, define o seu tamanho,
 * faz mmap, e inicializa todos os campos: tabuleiro vazio, mutex e
 * condition variables com PTHREAD_PROCESS_SHARED.
 *
 * @return Ponteiro para shared_data_t em caso de sucesso; NULL em caso de erro.
 */
shared_data_t *init_shared_memory(void)
{
    int fd;
    shared_data_t *shared;

    /* ---- Remover SHM anterior caso exista ---- */
    shm_unlink(SHM_NAME);

    /* ---- Criar a região de memória partilhada ---- */
    fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("[ERRO] shm_open falhou");
        return NULL;
    }

    /* ---- Definir o tamanho da região ---- */
    if (ftruncate(fd, (off_t)sizeof(shared_data_t)) == -1) {
        perror("[ERRO] ftruncate falhou");
        close(fd);
        shm_unlink(SHM_NAME);
        return NULL;
    }

    /* ---- Mapear em memória ---- */
    shared = (shared_data_t *)mmap(NULL, sizeof(shared_data_t),
                                   PROT_READ | PROT_WRITE,
                                   MAP_SHARED, fd, 0);
    if (shared == MAP_FAILED) {
        perror("[ERRO] mmap falhou");
        close(fd);
        shm_unlink(SHM_NAME);
        return NULL;
    }

    /* O file descriptor pode ser fechado após mmap */
    close(fd);

    /* ---- Inicializar dados do tabuleiro ---- */
    memset(shared->board, 0, sizeof(shared->board));
    shared->count = 0;
    shared->in = 0;
    shared->out = 0;
    shared->next_sample_id = 1;

    /* ---- Inicializar estado do sistema ---- */
    shared->analysis_active = TRUE;
    shared->terminate = FALSE;

    /* ---- Inicializar contadores estatísticos ---- */
    shared->total_created = 0;
    shared->total_deposited = 0;
    shared->total_analyzed = 0;
    shared->total_wait_full = 0;
    shared->total_wait_empty = 0;

    /* ---- Inicializar Semáforos com PTHREAD_PROCESS_SHARED ---- */
    if (sem_init(&shared->sem_mutex, 1, 1) != 0) {
        perror("[ERRO] sem_init (sem_mutex) falhou");
        munmap(shared, sizeof(shared_data_t));
        shm_unlink(SHM_NAME);
        return NULL;
    }

    if (sem_init(&shared->sem_empty, 1, BOARD_CAPACITY) != 0) {
        perror("[ERRO] sem_init (sem_empty) falhou");
        sem_destroy(&shared->sem_mutex);
        munmap(shared, sizeof(shared_data_t));
        shm_unlink(SHM_NAME);
        return NULL;
    }

    if (sem_init(&shared->sem_full, 1, 0) != 0) {
        perror("[ERRO] sem_init (sem_full) falhou");
        sem_destroy(&shared->sem_empty);
        sem_destroy(&shared->sem_mutex);
        munmap(shared, sizeof(shared_data_t));
        shm_unlink(SHM_NAME);
        return NULL;
    }

    if (sem_init(&shared->sem_analysis, 1, 0) != 0) {
        perror("[ERRO] sem_init (sem_analysis) falhou");
        sem_destroy(&shared->sem_full);
        sem_destroy(&shared->sem_empty);
        sem_destroy(&shared->sem_mutex);
        munmap(shared, sizeof(shared_data_t));
        shm_unlink(SHM_NAME);
        return NULL;
    }

    shared->num_waiting_analysis = 0;

    log_main("Shared memory criada e inicializada.");
    return shared;
}

/**
 * Abre a memória partilhada já existente (para processos filhos).
 *
 * @return Ponteiro para shared_data_t em caso de sucesso; NULL em caso de erro.
 */
shared_data_t *open_shared_memory(void)
{
    int fd;
    shared_data_t *shared;

    fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1) {
        perror("[ERRO] shm_open (open) falhou");
        return NULL;
    }

    shared = (shared_data_t *)mmap(NULL, sizeof(shared_data_t),
                                   PROT_READ | PROT_WRITE,
                                   MAP_SHARED, fd, 0);
    if (shared == MAP_FAILED) {
        perror("[ERRO] mmap (open) falhou");
        close(fd);
        return NULL;
    }

    close(fd);
    return shared;
}

/**
 * Liberta e remove a memória partilhada do sistema.
 *
 * Destrói mutex e condition variables, faz munmap e shm_unlink.
 * Imprime estatísticas finais antes de libertar.
 *
 * @param shared Ponteiro para a estrutura de dados partilhados.
 */
void cleanup_shared_memory(shared_data_t *shared)
{
    if (shared == NULL) {
        return;
    }

    /* ---- Imprimir estatísticas finais ---- */
    log_main("=== Estatísticas Finais ===");
    log_main("Amostras criadas:     %d", shared->total_created);
    log_main("Amostras depositadas: %d", shared->total_deposited);
    log_main("Amostras analisadas:  %d", shared->total_analyzed);
    log_main("Esperas (cheio):      %d", shared->total_wait_full);
    log_main("Esperas (vazio):      %d", shared->total_wait_empty);
    log_main("===========================");

    /* ---- Destruir mecanismos de sincronização ---- */
    if (sem_destroy(&shared->sem_analysis) != 0) {
        perror("[AVISO] sem_destroy (sem_analysis) falhou");
    }

    if (sem_destroy(&shared->sem_full) != 0) {
        perror("[AVISO] sem_destroy (sem_full) falhou");
    }

    if (sem_destroy(&shared->sem_empty) != 0) {
        perror("[AVISO] sem_destroy (sem_empty) falhou");
    }

    if (sem_destroy(&shared->sem_mutex) != 0) {
        perror("[AVISO] sem_destroy (sem_mutex) falhou");
    }

    /* ---- Desmapear e remover memória partilhada ---- */
    if (munmap(shared, sizeof(shared_data_t)) == -1) {
        perror("[AVISO] munmap falhou");
    }

    if (shm_unlink(SHM_NAME) == -1) {
        perror("[AVISO] shm_unlink falhou");
    }

    log_main("Shared memory libertada.");
}
