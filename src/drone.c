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
        
        /* Verificar se o tabuleiro está cheio para incrementar estatísticas */
        if (sem_trywait(&shared->sem_empty) != 0) {
            sem_wait(&shared->sem_mutex);
            shared->total_wait_full++;
            sem_post(&shared->sem_mutex);

            log_drone(id, "Tabuleiro cheio. A aguardar espaco...");
            
            // Aguarda slot livre (bloqueante, sem espera ativa)
            sem_wait(&shared->sem_empty);
        }

        /* Se o sistema estiver a terminar, propagamos e saímos */
        if (shared->terminate) {
            sem_post(&shared->sem_empty); // Propagar
            break;
        }

        if (sem_wait(&shared->sem_mutex) != 0) {
            perror("[ERRO] Drone: sem_wait (sem_mutex) falhou");
            break;
        }

        if (shared->terminate) {
            sem_post(&shared->sem_mutex);
            sem_post(&shared->sem_empty);
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

        if (sem_post(&shared->sem_mutex) != 0) {
            perror("[ERRO] Drone: sem_post (sem_mutex) falhou");
        }

        /* ---- Sinalizar analisadores que há amostra disponível ---- */
        if (sem_post(&shared->sem_full) != 0) {
            perror("[ERRO] Drone: sem_post (sem_full) falhou");
        }
    }

    log_drone(id, "Drone terminado.");
    return NULL;
}
