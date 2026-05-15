/**
 * @file logger.c
 * @brief Implementação do sistema de logging centralizado.
 *
 * Fornece funções thread-safe para imprimir mensagens com tabulação
 * diferenciada conforme o módulo de origem. Usa um mutex para evitar
 * que as mensagens de diferentes threads se misturem no output.
 */

#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <time.h>

#include "include/logger.h"
#include "include/macros.h"

/** Mutex para proteger o output (evitar mensagens entrelaçadas) */
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Obtém o timestamp atual formatado como [HH:MM:SS].
 *
 * @param buffer Buffer para armazenar o timestamp (mínimo 12 bytes).
 * @param size   Tamanho do buffer.
 */
static void get_timestamp(char *buffer, size_t size)
{
    time_t now;
    struct tm *tm_info;

    now = time(NULL);
    tm_info = localtime(&now);

    if (tm_info != NULL) {
        strftime(buffer, size, "[%H:%M:%S]", tm_info);
    } else {
        buffer[0] = '\0';
    }
}

/**
 * Imprime uma mensagem de log do módulo principal.
 * Formato: "\t[HH:MM:SS] [MAIN] mensagem"
 */
void log_main(const char *format, ...)
{
    va_list args;
    char timestamp[16];

    get_timestamp(timestamp, sizeof(timestamp));

    pthread_mutex_lock(&log_mutex);

    printf("%s%s [%s] ", TAB_MAIN, timestamp, MOD_MAIN);

    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
    fflush(stdout);

    pthread_mutex_unlock(&log_mutex);
}

/**
 * Imprime uma mensagem de log de um drone.
 * Formato: "\t\t\t[HH:MM:SS] [DRONE X] mensagem"
 */
void log_drone(int drone_id, const char *format, ...)
{
    va_list args;
    char timestamp[16];

    get_timestamp(timestamp, sizeof(timestamp));

    pthread_mutex_lock(&log_mutex);

    printf("%s%s [DRONE %d] ", TAB_DRONE, timestamp, drone_id);

    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
    fflush(stdout);

    pthread_mutex_unlock(&log_mutex);
}

/**
 * Imprime uma mensagem de log de um analisador.
 * Formato: "\t\t\t\t\t[HH:MM:SS] [ANALISADOR X] mensagem"
 */
void log_analyzer(int analyzer_id, const char *format, ...)
{
    va_list args;
    char timestamp[16];

    get_timestamp(timestamp, sizeof(timestamp));

    pthread_mutex_lock(&log_mutex);

    printf("%s%s [ANALISADOR %d] ", TAB_ANALYZER, timestamp, analyzer_id);

    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
    fflush(stdout);

    pthread_mutex_unlock(&log_mutex);
}
