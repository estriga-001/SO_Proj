/**
 * @file main.c
 * @brief Processo principal da Estação Autónoma de Exploração Planetária.
 *
 * Responsável por:
 * - Inicializar a memória partilhada.
 * - Instalar handlers de sinais (SIGINT, SIGTSTP).
 * - Criar o processo filho de exploração (drones).
 * - Criar o processo filho de análise científica (braços robóticos).
 * - Gerir sinais no loop principal:
 *   - SIGINT  → terminação limpa de todo o sistema.
 *   - SIGTSTP → toggle do estado de análise (ativo/standby).
 * - Esperar pelos processos filhos.
 * - Libertar todos os recursos.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>

#include "include/projeto.h"

/**
 * Ponto de entrada do programa.
 *
 * Fluxo principal:
 * 1. Inicializa memória partilhada.
 * 2. Instala handlers de sinais.
 * 3. fork() para o processo de exploração.
 * 4. fork() para o processo de análise.
 * 5. Loop principal: verifica flags de sinais e reage.
 * 6. Espera pelos processos filhos com waitpid.
 * 7. Liberta memória partilhada.
 *
 * @return EXIT_SUCCESS se tudo correr bem; EXIT_FAILURE em caso de erro.
 */
int main(void)
{
    shared_data_t *shared;
    pid_t pid_exploration;
    pid_t pid_analysis;
    int status;
    pid_t orig_pgrp = -1;

    /* ==================================================================
     * 0. CRIAR PROCESS GROUP PRÓPRIO
     *
     * Quando executado via 'make run', o make e o estacao partilham
     * o mesmo process group. Ctrl-Z (SIGTSTP) vai para todo o grupo,
     * e o make é parado (acção por omissão). Para evitar isso,
     * criamos o nosso próprio process group e tornamo-lo o foreground
     * group do terminal. Assim, Ctrl-Z só afeta o estacao.
     * ================================================================== */

    if (isatty(STDIN_FILENO)) {
        orig_pgrp = tcgetpgrp(STDIN_FILENO);
        setpgid(0, 0);
        /* Ignorar SIGTTOU para que tcsetpgrp não nos pare */
        signal(SIGTTOU, SIG_IGN);
        tcsetpgrp(STDIN_FILENO, getpid());
    }

    log_main("========================================");
    log_main("Estacao Autonoma de Exploracao Planetaria");
    log_main("========================================");
    log_main("Sistema a iniciar...");

    /* ==================================================================
     * 1. INICIALIZAR MEMÓRIA PARTILHADA
     * ================================================================== */

    shared = init_shared_memory();
    if (shared == NULL) {
        fprintf(stderr, "[ERRO FATAL] Nao foi possivel criar shared memory.\n");
        return EXIT_FAILURE;
    }

    /* ==================================================================
     * 2. INSTALAR HANDLERS DE SINAIS
     * ================================================================== */

    if (install_signal_handlers() != 0) {
        fprintf(stderr, "[ERRO FATAL] Nao foi possivel instalar handlers de sinais.\n");
        cleanup_shared_memory(shared);
        return EXIT_FAILURE;
    }

    log_main("Handlers de sinais instalados.");
    log_main("  Ctrl-C (SIGINT)  -> Terminar sistema");
    log_main("  Ctrl-Z (SIGTSTP) -> Ativar/Desativar analise");

    /* ==================================================================
     * 3. CRIAR PROCESSO DE EXPLORAÇÃO
     * ================================================================== */

    pid_exploration = fork();

    if (pid_exploration == -1) {
        perror("[ERRO FATAL] fork (exploracao) falhou");
        cleanup_shared_memory(shared);
        return EXIT_FAILURE;
    }

    if (pid_exploration == 0) {
        /* ---- Processo filho: exploração ---- */
        run_exploration(shared);
        /* O filho termina aqui */
        _exit(EXIT_SUCCESS);
    }

    log_main("Processo de exploracao criado (PID: %d).", (int)pid_exploration);

    /* ==================================================================
     * 4. CRIAR PROCESSO DE ANÁLISE CIENTÍFICA
     * ================================================================== */

    pid_analysis = fork();

    if (pid_analysis == -1) {
        perror("[ERRO FATAL] fork (analise) falhou");
        /* Terminar o processo de exploração já criado */
        shared->terminate = TRUE;
        pthread_cond_broadcast(&shared->can_deposit);
        kill(pid_exploration, SIGINT);
        waitpid(pid_exploration, &status, 0);
        cleanup_shared_memory(shared);
        return EXIT_FAILURE;
    }

    if (pid_analysis == 0) {
        /* ---- Processo filho: análise científica ---- */
        run_analysis(shared);
        /* O filho termina aqui */
        _exit(EXIT_SUCCESS);
    }

    log_main("Processo de analise criado (PID: %d).", (int)pid_analysis);
    log_main("Sistema iniciado. A aguardar sinais...");

    /* ==================================================================
     * 5. LOOP PRINCIPAL — GESTÃO DE SINAIS
     *
     * Usamos sigprocmask para bloquear SIGINT e SIGTSTP durante a
     * execução normal do loop, e sigsuspend para os desbloquear
     * atomicamente enquanto o processo está suspenso. Isto elimina
     * a race condition entre verificar as flags e suspender.
     * ================================================================== */

    sigset_t block_mask, wait_mask;

    /* Máscara de sinais a bloquear durante execução do loop */
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGINT);
    sigaddset(&block_mask, SIGTSTP);

    /* Máscara para sigsuspend: desbloqueia SIGINT e SIGTSTP */
    sigfillset(&wait_mask);
    sigdelset(&wait_mask, SIGINT);
    sigdelset(&wait_mask, SIGTSTP);

    /* Bloquear SIGINT e SIGTSTP — serão entregues apenas em sigsuspend */
    if (sigprocmask(SIG_BLOCK, &block_mask, NULL) == -1) {
        perror("[ERRO] sigprocmask falhou");
    }

    while (!got_sigint) {

        /* ---- Verificar se SIGTSTP foi recebido (toggle análise) ---- */
        if (got_sigtstp) {
            got_sigtstp = 0;

            /* Secção crítica: alterar estado da análise */
            if (pthread_mutex_lock(&shared->mutex) != 0) {
                perror("[ERRO] main: pthread_mutex_lock falhou");
                break;
            }

            /* Toggle do estado de análise */
            shared->analysis_active = !shared->analysis_active;

            if (shared->analysis_active) {
                log_main("SIGTSTP recebido. Sistema de analise ATIVADO.");
                /* Acordar braços que estavam em standby */
                pthread_cond_broadcast(&shared->can_analyze);
            } else {
                log_main("SIGTSTP recebido. Sistema de analise DESATIVADO.");
            }

            if (pthread_mutex_unlock(&shared->mutex) != 0) {
                perror("[ERRO] main: pthread_mutex_unlock falhou");
                break;
            }
        }

        /* Suspender atomicamente, desbloqueando SIGINT e SIGTSTP.
         * sigsuspend() retorna quando um sinal é entregue, garantindo
         * que nenhum sinal é perdido (sem race condition). */
        sigsuspend(&wait_mask);
    }

    /* ==================================================================
     * 6. TERMINAÇÃO LIMPA (SIGINT recebido)
     * ================================================================== */

    log_main("SIGINT recebido. A terminar sistema...");

    /* ---- Sinalizar todos os módulos para terminar ---- */
    if (pthread_mutex_lock(&shared->mutex) != 0) {
        perror("[ERRO] main: pthread_mutex_lock (terminate) falhou");
    }

    shared->terminate = TRUE;

    /* Acordar todas as threads que possam estar bloqueadas */
    pthread_cond_broadcast(&shared->can_deposit);
    pthread_cond_broadcast(&shared->can_analyze);

    if (pthread_mutex_unlock(&shared->mutex) != 0) {
        perror("[ERRO] main: pthread_mutex_unlock (terminate) falhou");
    }

    /* Enviar SIGINT aos processos filhos para garantir que acordam de sleep() */
    kill(pid_exploration, SIGINT);
    kill(pid_analysis, SIGINT);

    /* ==================================================================
     * 7. ESPERAR PELOS PROCESSOS FILHOS
     * ================================================================== */

    log_main("A aguardar processo de exploracao (PID: %d)...",
             (int)pid_exploration);
    if (waitpid(pid_exploration, &status, 0) == -1) {
        perror("[AVISO] waitpid (exploracao) falhou");
    }

    log_main("A aguardar processo de analise (PID: %d)...",
             (int)pid_analysis);
    if (waitpid(pid_analysis, &status, 0) == -1) {
        perror("[AVISO] waitpid (analise) falhou");
    }

    log_main("Todos os processos filhos terminaram.");

    /* ==================================================================
     * 8. LIBERTAR RECURSOS
     * ================================================================== */

    cleanup_shared_memory(shared);

    /* Restaurar o foreground process group original do terminal */
    if (orig_pgrp > 0 && isatty(STDIN_FILENO)) {
        tcsetpgrp(STDIN_FILENO, orig_pgrp);
    }

    log_main("Recursos libertados. Fim.");
    log_main("========================================");

    return EXIT_SUCCESS;
}
