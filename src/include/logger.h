/**
 * @file logger.h
 * @brief Protótipos para o sistema de logging centralizado.
 *
 * Fornece funções de log com tabulação diferenciada para cada
 * módulo do sistema, conforme especificado no enunciado.
 */

#ifndef LOGGER_H
#define LOGGER_H

/**
 * Imprime uma mensagem de log do módulo principal.
 * Usa tabulação: "\t"
 *
 * @param format String de formato (estilo printf).
 * @param ... Argumentos variáveis.
 */
void log_main(const char *format, ...);

/**
 * Imprime uma mensagem de log de um drone.
 * Usa tabulação: "\t\t\t"
 *
 * @param drone_id Identificador do drone.
 * @param format   String de formato (estilo printf).
 * @param ...      Argumentos variáveis.
 */
void log_drone(int drone_id, const char *format, ...);

/**
 * Imprime uma mensagem de log de um analisador.
 * Usa tabulação: "\t\t\t\t\t"
 *
 * @param analyzer_id Identificador do analisador.
 * @param format      String de formato (estilo printf).
 * @param ...         Argumentos variáveis.
 */
void log_analyzer(int analyzer_id, const char *format, ...);

#endif /* LOGGER_H */
