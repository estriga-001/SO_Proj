/**
 * @file analysis.c
 * @brief Implementação do módulo de análise científica (processo filho).
 *
 * Este módulo é executado como processo filho do processo principal.
 * Cria NUM_ANALYZERS threads de braços robóticos / aparelhos de análise,
 * espera pela sua terminação e retorna.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include "include/analysis.h"
#include "include/robotic_arm.h"
#include "include/logger.h"

/**
 * Função principal do processo de análise científica.
 *
 * 1. Bloqueia SIGTSTP (apenas o pai trata o toggle de análise).
 * 2. Inicializa o gerador de números aleatórios (srand).
 * 3. Cria NUM_ANALYZERS threads de braços robóticos.
 * 4. Cada thread recebe o seu ID e o ponteiro para os dados partilhados.
 * 5. Espera pela terminação de todas as threads com pthread_join.
 *
 * @param shared Ponteiro para os dados partilhados (memória partilhada).
 */
void run_analysis(shared_data_t *shared)
{
    /* Bloquear SIGTSTP neste processo filho — apenas o pai faz toggle */
    sigset_t block_tstp;
    sigemptyset(&block_tstp);
    sigaddset(&block_tstp, SIGTSTP);
    sigprocmask(SIG_BLOCK, &block_tstp, NULL);
    pthread_t threads[NUM_ANALYZERS];
    analyzer_args_t args[NUM_ANALYZERS];
    int i;

    /* Inicializar srand para tempos de análise aleatórios */
    srand((unsigned int)(time(NULL) ^ getpid()));

    log_main("Modulo de analise cientifica iniciado (PID: %d).", (int)getpid());

    /* ---- Criar threads dos analisadores ---- */
    for (i = 0; i < NUM_ANALYZERS; i++) {
        args[i].analyzer_id = i + 1;
        args[i].shared = shared;

        if (pthread_create(&threads[i], NULL, analyzer_thread, &args[i]) != 0) {
            perror("[ERRO] pthread_create (analisador) falhou");
            fprintf(stderr, "[ERRO] Nao foi possivel criar analisador %d\n", i + 1);
            /* Terminar as threads já criadas */
            sem_wait(&shared->sem_mutex);
            shared->terminate = TRUE;
            int to_wake = shared->num_waiting_analysis;
            shared->num_waiting_analysis = 0;
            sem_post(&shared->sem_mutex);

            for (int k = 0; k < to_wake; k++) {
                sem_post(&shared->sem_analysis);
            }
            for (int k = 0; k < NUM_ANALYZERS; k++) {
                sem_post(&shared->sem_full);
            }
            for (int j = 0; j < i; j++) {
                pthread_join(threads[j], NULL);
            }
            return;
        }

        log_main("Analisador %d criado.", i + 1);
    }

    /* ---- Esperar pela terminação de todas as threads ---- */
    for (i = 0; i < NUM_ANALYZERS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("[ERRO] pthread_join (analisador) falhou");
        }
    }

    log_main("Modulo de analise cientifica terminado.");
}
