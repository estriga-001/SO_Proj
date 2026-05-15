/**
 * @file exploration.c
 * @brief Implementação do módulo de exploração (processo filho).
 *
 * Este módulo é executado como processo filho do processo principal.
 * Cria NUM_DRONES threads de drones exploratórios, espera pela sua
 * terminação e retorna.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include "include/exploration.h"
#include "include/drone.h"
#include "include/logger.h"

/**
 * Função principal do processo de exploração.
 *
 * 1. Bloqueia SIGTSTP (apenas o pai trata o toggle de análise).
 * 2. Cria NUM_DRONES threads de drones.
 * 3. Cada thread recebe o seu ID e o ponteiro para os dados partilhados.
 * 4. Espera pela terminação de todas as threads com pthread_join.
 *
 * @param shared Ponteiro para os dados partilhados (memória partilhada).
 */
void run_exploration(shared_data_t *shared)
{
    /* Bloquear SIGTSTP neste processo filho — apenas o pai faz toggle */
    sigset_t block_tstp;
    sigemptyset(&block_tstp);
    sigaddset(&block_tstp, SIGTSTP);
    sigprocmask(SIG_BLOCK, &block_tstp, NULL);
    pthread_t threads[NUM_DRONES];
    drone_args_t args[NUM_DRONES];
    int i;

    log_main("Modulo de exploracao iniciado (PID: %d).", (int)getpid());

    /* ---- Criar threads dos drones ---- */
    for (i = 0; i < NUM_DRONES; i++) {
        args[i].drone_id = i + 1;
        args[i].shared = shared;

        if (pthread_create(&threads[i], NULL, drone_thread, &args[i]) != 0) {
            perror("[ERRO] pthread_create (drone) falhou");
            fprintf(stderr, "[ERRO] Nao foi possivel criar drone %d\n", i + 1);
            /* Terminar as threads já criadas */
            shared->terminate = TRUE;
            pthread_cond_broadcast(&shared->can_deposit);
            for (int j = 0; j < i; j++) {
                pthread_join(threads[j], NULL);
            }
            return;
        }

        log_main("Drone %d criado.", i + 1);
    }

    /* ---- Esperar pela terminação de todas as threads ---- */
    for (i = 0; i < NUM_DRONES; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("[ERRO] pthread_join (drone) falhou");
        }
    }

    log_main("Modulo de exploracao terminado.");
}
