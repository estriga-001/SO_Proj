/**
 * @file exploration.h
 * @brief Protótipo para o módulo de exploração (processo filho).
 *
 * O módulo de exploração é um processo filho que cria e gere
 * as threads dos drones exploratórios.
 */

#ifndef EXPLORATION_H
#define EXPLORATION_H

#include "shared_memory.h"

/**
 * Função principal do processo de exploração.
 *
 * Cria NUM_DRONES threads de drones, espera pela sua terminação
 * com pthread_join e retorna.
 *
 * @param shared Ponteiro para os dados partilhados (memória partilhada).
 */
void run_exploration(shared_data_t *shared);

#endif /* EXPLORATION_H */
