/**
 * @file macros.h
 * @brief Constantes globais do projeto Estação Autónoma de Exploração Planetária.
 *
 * Define todas as constantes de configuração utilizadas pelo sistema,
 * incluindo capacidades, tempos, nomes de recursos e estados.
 */

#ifndef MACROS_H
#define MACROS_H

/* ============================================================
 * Configuração do tabuleiro de amostras
 * ============================================================ */

/** Capacidade máxima do tabuleiro (buffer circular) */
#define BOARD_CAPACITY      10

/* ============================================================
 * Configuração dos drones e analisadores
 * ============================================================ */

/** Número de drones exploratórios */
#define NUM_DRONES          3

/** Número de braços robóticos / aparelhos de análise */
#define NUM_ANALYZERS       2

/** Tempo (segundos) entre recolhas de amostras por cada drone */
#define DRONE_DELIVERY_TIME 5

/** Tempo mínimo de análise de uma amostra (segundos) */
#define ANALYSIS_MIN_TIME   1

/** Tempo máximo de análise de uma amostra (segundos) */
#define ANALYSIS_MAX_TIME   3

/* ============================================================
 * Memória partilhada
 * ============================================================ */

/** Nome da região de memória partilhada POSIX */
#define SHM_NAME            "/estacao_planetaria_shm"

/* ============================================================
 * Estados booleanos
 * ============================================================ */

#define TRUE                1
#define FALSE               0

/* ============================================================
 * Códigos de erro
 * ============================================================ */

#define ERR_SHM_OPEN        1
#define ERR_FTRUNCATE       2
#define ERR_MMAP            3
#define ERR_MUTEX_INIT      4
#define ERR_COND_INIT       5
#define ERR_FORK            6
#define ERR_THREAD_CREATE   7
#define ERR_SIGACTION       8

/* ============================================================
 * Nomes dos módulos (para logging)
 * ============================================================ */

#define MOD_MAIN            "MAIN"
#define MOD_EXPLORATION     "EXPLORAÇÃO"
#define MOD_ANALYSIS        "ANÁLISE"

/* ============================================================
 * Tabulações para output conforme enunciado
 * ============================================================ */

/** Tabulação do módulo principal */
#define TAB_MAIN            "\t"

/** Tabulação dos drones */
#define TAB_DRONE           "\t\t\t"

/** Tabulação dos analisadores */
#define TAB_ANALYZER        "\t\t\t\t\t"

#endif /* MACROS_H */
