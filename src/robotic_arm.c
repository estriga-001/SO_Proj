/**
 * @file robotic_arm.c
 * @brief Implementação da thread do braço robótico / analisador.
 *
 * Cada braço robótico retira uma amostra do tabuleiro, analisa-a
 * durante um tempo aleatório (1-3 segundos) e descarta-a.
 * Se o tabuleiro estiver vazio ou a análise estiver desativada,
 * o braço espera em condition variable.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "include/robotic_arm.h"
#include "include/logger.h"

/**
 * Função principal da thread de um braço robótico / analisador.
 *
 * Ciclo de vida:
 * 1. Adquire o mutex.
 * 2. Enquanto (tabuleiro vazio OU análise desativada) e sistema não a terminar:
 *    - Espera em can_analyze (condition variable).
 * 3. Se sistema a terminar: liberta mutex e termina.
 * 4. Retira amostra do tabuleiro, atualiza contadores.
 * 5. Sinaliza can_deposit para acordar drones.
 * 6. Liberta mutex.
 * 7. Analisa amostra fora da secção crítica (1-3 segundos).
 * 8. Descarta amostra e volta ao passo 1.
 *
 * @param arg Ponteiro para analyzer_args_t com ID e dados partilhados.
 * @return NULL.
 */
void *analyzer_thread(void *arg)
{
    analyzer_args_t *args = (analyzer_args_t *)arg;
    int id = args->analyzer_id;
    shared_data_t *shared = args->shared;
    sample_t sample;
    int analysis_time;
    int logged_standby = 0; /* Evitar spam de mensagens de standby */

    log_analyzer(id, "Analisador iniciado.");

    while (1) {
        /* ---- Secção crítica: retirar amostra do tabuleiro ---- */
        if (pthread_mutex_lock(&shared->mutex) != 0) {
            perror("[ERRO] Analisador: pthread_mutex_lock falhou");
            break;
        }

        /* Esperar enquanto (vazio OU análise desativada) e não terminar */
        while (!shared->terminate &&
               (shared->count == 0 || !shared->analysis_active)) {

            /* Mensagem de standby (apenas uma vez por ciclo) */
            if (!shared->analysis_active && !logged_standby) {
                log_analyzer(id, "Sistema de analise em standby.");
                logged_standby = 1;
            }

            if (shared->count == 0 && shared->analysis_active) {
                shared->total_wait_empty++;
            }

            if (pthread_cond_wait(&shared->can_analyze, &shared->mutex) != 0) {
                perror("[ERRO] Analisador: pthread_cond_wait falhou");
                pthread_mutex_unlock(&shared->mutex);
                return NULL;
            }
        }

        /* Reset da flag de standby quando volta a analisar */
        logged_standby = 0;

        /* Verificar se devemos terminar */
        if (shared->terminate) {
            pthread_mutex_unlock(&shared->mutex);
            break;
        }

        /* ---- Retirar amostra do tabuleiro ---- */
        sample = shared->board[shared->out];
        shared->out = (shared->out + 1) % BOARD_CAPACITY;
        shared->count--;

        log_analyzer(id, "Amostra %d retirada do tabuleiro. Ocupacao: %d/%d.",
                     sample.id, shared->count, BOARD_CAPACITY);

        /* ---- Sinalizar drones que há espaço disponível ---- */
        if (pthread_cond_broadcast(&shared->can_deposit) != 0) {
            perror("[ERRO] Analisador: pthread_cond_broadcast falhou");
        }

        if (pthread_mutex_unlock(&shared->mutex) != 0) {
            perror("[ERRO] Analisador: pthread_mutex_unlock falhou");
        }

        /* ---- Analisar amostra FORA da secção crítica ---- */
        analysis_time = (rand() % (ANALYSIS_MAX_TIME - ANALYSIS_MIN_TIME + 1))
                        + ANALYSIS_MIN_TIME;

        log_analyzer(id, "A analisar amostra %d durante %d segundo(s).",
                     sample.id, analysis_time);

        sleep((unsigned int)analysis_time);

        /* ---- Atualizar contador de analisadas (secção crítica breve) ---- */
        if (pthread_mutex_lock(&shared->mutex) != 0) {
            perror("[ERRO] Analisador: pthread_mutex_lock (stats) falhou");
            break;
        }

        shared->total_analyzed++;

        if (pthread_mutex_unlock(&shared->mutex) != 0) {
            perror("[ERRO] Analisador: pthread_mutex_unlock (stats) falhou");
        }

        log_analyzer(id, "Amostra %d analisada e descartada.", sample.id);
    }

    log_analyzer(id, "Analisador terminado.");
    return NULL;
}
