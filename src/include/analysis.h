/**
 * @file analysis.h
 * @brief Protótipo para o módulo de análise científica (processo filho).
 *
 * O módulo de análise é um processo filho que cria e gere
 * as threads dos braços robóticos / aparelhos de análise.
 */

#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "shared_memory.h"

/**
 * Função principal do processo de análise científica.
 *
 * Cria NUM_ANALYZERS threads de braços robóticos, espera pela sua
 * terminação com pthread_join e retorna.
 *
 * @param shared Ponteiro para os dados partilhados (memória partilhada).
 */
void run_analysis(shared_data_t *shared);

#endif /* ANALYSIS_H */
