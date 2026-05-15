/**
 * @file drone.h
 * @brief Protótipos e estruturas para as threads dos drones.
 *
 * Define a estrutura de argumentos passada a cada thread de drone
 * e o protótipo da função de thread.
 */

#ifndef DRONE_H
#define DRONE_H

#include "shared_memory.h"

/**
 * Estrutura de argumentos para a thread de um drone.
 */
typedef struct {
    int drone_id;           /**< Identificador do drone (1 a NUM_DRONES) */
    shared_data_t *shared;  /**< Ponteiro para os dados partilhados */
} drone_args_t;

/**
 * Função principal da thread de um drone.
 *
 * Ciclo de vida:
 * 1. Espera DRONE_DELIVERY_TIME segundos (simula recolha).
 * 2. Tenta depositar amostra no tabuleiro.
 * 3. Se tabuleiro cheio, espera em condition variable.
 * 4. Deposita amostra e sinaliza os analisadores.
 * 5. Repete até receber sinal de terminação.
 *
 * @param arg Ponteiro para drone_args_t.
 * @return NULL.
 */
void *drone_thread(void *arg);

#endif /* DRONE_H */
