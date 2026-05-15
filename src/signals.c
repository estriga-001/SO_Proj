/**
 * @file signals.c
 * @brief Implementação do tratamento de sinais do sistema.
 *
 * Instala handlers para SIGINT (Ctrl-C) e SIGTSTP (Ctrl-Z).
 * Os handlers apenas setam flags globais do tipo volatile sig_atomic_t,
 * sem executar lógica complexa dentro do handler.
 */

#include <stdio.h>
#include <string.h>

#include "include/signals.h"
#include "include/macros.h"

/* ============================================================
 * Flags globais de sinais
 * ============================================================ */

/** Flag que indica que SIGINT foi recebido */
volatile sig_atomic_t got_sigint = 0;

/** Flag que indica que SIGTSTP foi recebido */
volatile sig_atomic_t got_sigtstp = 0;

/* ============================================================
 * Handlers de sinais
 * ============================================================ */

/**
 * Handler para SIGINT (Ctrl-C).
 *
 * Apenas seta a flag got_sigint para 1.
 * A lógica de terminação é tratada no fluxo principal.
 *
 * @param sig Número do sinal recebido (não utilizado diretamente).
 */
static void sigint_handler(int sig)
{
    (void)sig; /* Evitar warning de parâmetro não usado */
    got_sigint = 1;
}

/**
 * Handler para SIGTSTP (Ctrl-Z).
 *
 * Apenas seta a flag got_sigtstp para 1.
 * O toggle da análise é tratado no fluxo principal.
 *
 * @param sig Número do sinal recebido (não utilizado diretamente).
 */
static void sigtstp_handler(int sig)
{
    (void)sig; /* Evitar warning de parâmetro não usado */
    got_sigtstp = 1;
}

/* ============================================================
 * Instalação dos handlers
 * ============================================================ */

/**
 * Instala os handlers de sinais para SIGINT e SIGTSTP.
 *
 * Usa sigaction() conforme boas práticas POSIX.
 * - SIGINT: terminação limpa do sistema.
 * - SIGTSTP: toggle do estado de análise (ativo/standby).
 *
 * @return 0 em caso de sucesso; -1 em caso de erro.
 */
int install_signal_handlers(void)
{
    struct sigaction sa_int;
    struct sigaction sa_tstp;

    /* ---- Configurar handler para SIGINT ---- */
    memset(&sa_int, 0, sizeof(sa_int));
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0; /* Sem SA_RESTART para que syscalls bloqueantes retornem */

    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("[ERRO] sigaction SIGINT falhou");
        return -1;
    }

    /* ---- Configurar handler para SIGTSTP ---- */
    memset(&sa_tstp, 0, sizeof(sa_tstp));
    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = 0;

    if (sigaction(SIGTSTP, &sa_tstp, NULL) == -1) {
        perror("[ERRO] sigaction SIGTSTP falhou");
        return -1;
    }

    return 0;
}
