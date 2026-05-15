/**
 * @file drone.c
 * @brief Implementação da thread do drone exploratório.
 *
 * Cada drone recolhe amostras periodicamente e deposita-as no tabuleiro.
 * Se o tabuleiro estiver cheio, o drone espera em condition variable
 * até haver espaço disponível.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "include/drone.h"
#include "include/logger.h"

/**
 * Função principal da thread de um drone.
 *
 * Ciclo de vida:
 * 1. Dorme DRONE_DELIVERY_TIME segundos (simula recolha de amostra).
 * 2. Adquire o mutex.
 * 3. Enquanto tabuleiro cheio e sistema não a terminar:
 *    - Espera em can_deposit (condition variable).
 * 4. Se sistema a terminar: liberta mutex e termina.
 * 5. Cria amostra, deposita no tabuleiro, atualiza contadores.
 * 6. Sinaliza can_analyze para acordar analisadores.
 * 7. Liberta mutex e volta ao passo 1.
 *
 * @param arg Ponteiro para drone_args_t com ID e dados partilhados.
 * @return NULL.
 */
void *drone_thread(void *arg)
{
    drone_args_t *args = (drone_args_t *)arg;
    int id = args->drone_id;
    shared_data_t *shared = args->shared;
    sample_t sample;

    log_drone(id, "Drone iniciado.");

    while (1) {
        /* ---- Simular tempo de recolha da amostra ---- */
        sleep(DRONE_DELIVERY_TIME);

        /* Verificar terminação antes de tentar depositar */
        if (shared->terminate) {
            break;
        }

        /* ---- Secção crítica: depositar amostra no tabuleiro ---- */
        if (pthread_mutex_lock(&shared->mutex) != 0) {
            perror("[ERRO] Drone: pthread_mutex_lock falhou");
            break;
        }

        /* Esperar enquanto tabuleiro cheio e sistema não a terminar */
        while (shared->count == BOARD_CAPACITY && !shared->terminate) {
            log_drone(id, "Tabuleiro cheio. A aguardar espaco...");
            shared->total_wait_full++;
            if (pthread_cond_wait(&shared->can_deposit, &shared->mutex) != 0) {
                perror("[ERRO] Drone: pthread_cond_wait falhou");
                pthread_mutex_unlock(&shared->mutex);
                return NULL;
            }
        }

        /* Verificar se devemos terminar */
        if (shared->terminate) {
            pthread_mutex_unlock(&shared->mutex);
            break;
        }

        /* ---- Criar e depositar a amostra ---- */
        sample.id = shared->next_sample_id++;
        sample.drone_id = id;
        sample.created_at = time(NULL);

        shared->total_created++;

        shared->board[shared->in] = sample;
        shared->in = (shared->in + 1) % BOARD_CAPACITY;
        shared->count++;
        shared->total_deposited++;

        log_drone(id, "Amostra %d recolhida e depositada no tabuleiro. "
                      "Ocupacao: %d/%d.",
                  sample.id, shared->count, BOARD_CAPACITY);

        /* ---- Sinalizar analisadores que há amostra disponível ---- */
        if (pthread_cond_broadcast(&shared->can_analyze) != 0) {
            perror("[ERRO] Drone: pthread_cond_broadcast falhou");
        }

        if (pthread_mutex_unlock(&shared->mutex) != 0) {
            perror("[ERRO] Drone: pthread_mutex_unlock falhou");
        }
    }

    log_drone(id, "Drone terminado.");
    return NULL;
}
