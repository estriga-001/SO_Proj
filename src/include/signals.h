/**
 * @file signals.h
 * @brief Protótipos e variáveis para o tratamento de sinais.
 *
 * Define as flags globais para SIGINT e SIGTSTP e o protótipo
 * para a instalação dos handlers de sinais.
 */

#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

/**
 * Flag global que indica se SIGINT (Ctrl-C) foi recebido.
 * Tipo volatile sig_atomic_t para segurança em signal handlers.
 */
extern volatile sig_atomic_t got_sigint;

/**
 * Flag global que indica se SIGTSTP (Ctrl-Z) foi recebido.
 * Tipo volatile sig_atomic_t para segurança em signal handlers.
 */
extern volatile sig_atomic_t got_sigtstp;

/**
 * Instala os handlers de sinais para SIGINT e SIGTSTP.
 *
 * Usa sigaction() para registar handlers que apenas setam
 * flags globais, sem lógica complexa dentro do handler.
 *
 * @return 0 em caso de sucesso; -1 em caso de erro.
 *
 * Possíveis erros:
 * - sigaction falhar para SIGINT ou SIGTSTP.
 */
int install_signal_handlers(void);

#endif /* SIGNALS_H */
