/**
 * @file robotic_arm.h
 * @brief Protótipos e estruturas para as threads dos braços robóticos.
 *
 * Define a estrutura de argumentos passada a cada thread de analisador
 * e o protótipo da função de thread.
 */

#ifndef ROBOTIC_ARM_H
#define ROBOTIC_ARM_H

#include "shared_memory.h"

/**
 * Estrutura de argumentos para a thread de um braço robótico.
 */
typedef struct {
    int analyzer_id;        /**< Identificador do analisador (1 a NUM_ANALYZERS) */
    shared_data_t *shared;  /**< Ponteiro para os dados partilhados */
} analyzer_args_t;

/**
 * Função principal da thread de um braço robótico / analisador.
 *
 * Ciclo de vida:
 * 1. Espera até haver amostras no tabuleiro E análise estar ativa.
 * 2. Retira uma amostra do tabuleiro.
 * 3. Sinaliza os drones que há espaço.
 * 4. Analisa a amostra fora da secção crítica (1-3 segundos).
 * 5. Descarta a amostra.
 * 6. Repete até receber sinal de terminação.
 *
 * @param arg Ponteiro para analyzer_args_t.
 * @return NULL.
 */
void *analyzer_thread(void *arg);

#endif /* ROBOTIC_ARM_H */
