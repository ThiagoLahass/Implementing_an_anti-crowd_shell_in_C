#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>

#define MAX_LINE_LENGTH 1000
#define MAX_COMMAND_LENGTH 200
#define MAX_EXTERNAL_COMMANDS 5
#define MAX_INTERNAL_COMMANDS 1
#define MAX_ARGS 3
#define FOREGROUND '%'
#define MAX_SESSIONS 1000

static void trim(char *str);

void child_death_handler(int n);
void ctrl_handler(int n);
void ctrl_handler_fg(int n);
void sigusr1_handler(int n);
void removeSessionArray(pid_t* sessions, pid_t sid, int current_session_index);

int main( int argc, char* argv[]){
    char* line_commands = calloc(MAX_LINE_LENGTH, sizeof(char));            // Comando de entrada do usuario no shell
    char** commands = calloc(MAX_EXTERNAL_COMMANDS, sizeof(char*));         // Vetor de comandos ( no maximo MAX_COMMANDS)
    for(int i = 0; i < MAX_EXTERNAL_COMMANDS; i++ ){
        commands[i] = calloc(MAX_COMMAND_LENGTH, sizeof(char));             // Comandos em si
    }
    const char delimiter[] = "<3";                                          // Delimitador usado

    pid_t* sessions = calloc(MAX_SESSIONS, sizeof(pid_t));                  // Array para guardar as sessões atuais
    int current_session_index = 0;

    // Instalação do tratador de ctrl+... do acsh
    struct sigaction sa;
    sa.sa_handler = ctrl_handler;
    sa.sa_flags = 0;

    if ((sigemptyset(&sa.sa_mask) == -1) ||
       (sigaction(SIGINT, &sa, NULL) == -1) ||
       (sigaction(SIGQUIT, &sa, NULL) == -1) ||
       (sigaction(SIGTSTP, &sa, NULL) == -1))
            perror("Failed to set SIGINT || SIGQUIT || SIGTSTP to handle Ctrl-...");

    while (1){
        free(line_commands);
        line_commands = calloc(MAX_LINE_LENGTH, sizeof(char));              // Comando de entrada do usuario no shell

        for(int i = 0; i < MAX_EXTERNAL_COMMANDS; i++ ){
            free(commands[i]);
        }
        free(commands);
        commands = calloc(MAX_EXTERNAL_COMMANDS, sizeof(char*));            // Vetor de comandos ( no maximo MAX_COMMANDS)
        for(int i = 0; i < MAX_EXTERNAL_COMMANDS; i++ ){
            commands[i] = calloc(MAX_COMMAND_LENGTH, sizeof(char));         // Comandos em si
        }


        int flag_input_valido = 1;
        int flag_foreground = 0;

        // Exibicao do prompt
        printf("acsh> ");

        /*=============== LEITURA DO COMANDO DE ENTRADA E SEPARACAO DOS COMANDOS ===============*/
        // Aguarda o comando do usuario e le ele quando inserido 
        fgets(line_commands, MAX_LINE_LENGTH, stdin);
        trim(line_commands);

        // Removendo o caractere nova linha ('\n') do final do comando e substitui por \0
        line_commands[strcspn(line_commands, "\n")] = '\0';

        // Sair do shell se o usuario digitar exit
        if (strcmp(line_commands, "exit") == 0) {
            int i = 0;
            for(i = 0; i < current_session_index; i++){             // Mata os processos de todas as sessões
                if (sessions[i] > 0 && sessions[i] != getsid(0)) {
                    kill(-sessions[i], SIGTERM);
                }
            }
            break;
        }
        
        char *token;
        char *remaining = line_commands;
        int command_count = 0;                              // Numero de comandos atual inserido pelo usuario
        int internal_command_count = 0;                     // Numero de comandos internos atual computados
        int external_command_count = 0;                     // Numero de comandos externos atual computados

        /* função strstr() é usada para localizar a próxima ocorrência do delimitador na string*/
        while ((token = strstr(remaining, delimiter)) != NULL) {

            /*Numero de comandos é maior que o permitido*/
            if( command_count+1 > MAX_EXTERNAL_COMMANDS ){
                fprintf(stderr, "ERRO: Numero de comandos é maior que o permitido\n");
                flag_input_valido = 0;
                break;
            }

            /*Em seguida, é calculado o comprimento da substring antes do delimitador*/
            int length = token - remaining;

            /*A função strncpy() é usada para copiar essa substring de tamanho length e salvar comando na string de comandos*/
            strncpy(commands[command_count], remaining, length);
            
            /*Remover espacos em branco no comeco e no final*/
            trim(commands[command_count]);

            //verificar se é comando interno
            if(strncmp(commands[command_count], "cd", 2) == 0 || strncmp(commands[command_count], "exit", 4) == 0){
                internal_command_count++;
                if(external_command_count > 0 ){
                    fprintf(stderr, "ERRO: Não pode haver comandos internos e externos em uma mesma linha\n");
                    flag_input_valido = 0;
                    break;
                }
                else if( internal_command_count > 1 ){
                    fprintf(stderr, "ERRO:Só pode haver um comando interno em uma linha\n");
                    flag_input_valido = 0;
                    break;
                }
            }
            else{
                external_command_count++;
                if(internal_command_count > 0 ){
                    fprintf(stderr, "ERRO: Não pode haver comandos internos e externos em uma mesma linha\n");
                    flag_input_valido = 0;
                    break;
                }
                if( external_command_count > 5 ){
                    fprintf(stderr, "ERRO: O numero de máximo de comandos externos em uma linha é 5\n");
                    flag_input_valido = 0;
                    break;
                }
            }

            /*Incrementar numero de comandos atual*/
            command_count++;
            
            /*Por fim, a variável remaining é atualizada para apontar para o próximo caractere após o delimitador*/
            remaining = token + strlen(delimiter);
        }

        if( flag_input_valido == 1 ){
            /*Numero de comandos é maior que o permitido*/
            if( command_count+1 > MAX_EXTERNAL_COMMANDS ){
                fprintf(stderr, "ERRO: Numero de comandos é maior que o permitido\n");
                flag_input_valido = 0;
            }
        }
        
        if( flag_input_valido == 1 ){
            /*Salvar ultimo comando na string de comandos*/
            strcpy(commands[command_count], remaining);

            /*Remover espacos em branco no começo e no final*/
            trim(commands[command_count]);

            /* Verificação de comandos internos/externos */
            if(strncmp(commands[command_count], "cd", 2) == 0 || strncmp(commands[command_count], "exit", 4) == 0){
                internal_command_count++;
                if(external_command_count > 0 ){
                    fprintf(stderr, "ERRO: Não pode haver comandos internos e externos em uma mesma linha\n");
                    flag_input_valido = 0;
                }
                else if( internal_command_count > 1 ){
                    fprintf(stderr, "ERRO:Só pode haver um comando interno em uma linha\n");
                    flag_input_valido = 0;
                }
            }
            else{
                external_command_count++;
                if(internal_command_count > 0 ){
                    fprintf(stderr, "ERRO: Não pode haver comandos internos e externos em uma mesma linha\n");
                    flag_input_valido = 0;
                }
                if( external_command_count > 5 ){
                    fprintf(stderr, "ERRO: O numero de máximo de comandos externos em uma linha é 5\n");
                    flag_input_valido = 0;
                }
            }

            /*Incrementar numero de comandos atual*/
            command_count++;
        }
        /*============= FIM (LEITURA DO COMANDO DE ENTRADA E SEPARACAO DOS COMANDOS) =============*/

        if(strcmp(commands[command_count-1],"") == 0) continue;

        /*============================= PROCESSAMENTO DOS COMANDOS ===============================*/
        if( flag_input_valido == 1 ){

            /*Verificação de foreground*/
            flag_foreground = 0;
            if(commands[command_count-1][strlen(commands[command_count-1])-1] == FOREGROUND){
                flag_foreground = 1;
                if(command_count > 1){
                    fprintf(stderr, "ERRO: Não pode é possível rodar mais de um programa em foreground\n");
                    flag_input_valido = 0;
                    continue;
                }
                if(internal_command_count == 1){
                    fprintf(stderr, "WARNING: Comandos internos já são executados em foreground!\nContinuando a execução...\n");
                }
            }

            pid_t pid = 1;

            // Se for comando externo(s) e não for comando foreground, criar um processo intermediario, para ser o lider da nova sessão
            if( internal_command_count == 0 && flag_foreground == 0){
                // Criação de um processo filho intermediario para o caso de background (processos filhos em uma sessao diferente)
                pid = fork();

                if (pid == 0) {                                                     // Processo filho
                    if (setsid() < 0) {                                             // Muda o session id do processo filho
                        fprintf(stderr, "Falha ao iniciar uma nova sessão para o processo filho.\n");
                        exit(1);
                    }
                    //===== Ignorar sinais de ctrl+... =====//
                    sa.sa_handler = ctrl_handler_fg;

                    if ((sigaction(SIGINT, &sa, NULL) == -1) ||
                        (sigaction(SIGQUIT, &sa, NULL) == -1) ||
                        (sigaction(SIGTSTP, &sa, NULL) == -1))
                            perror("Failed to set SIGINT || SIGQUIT || SIGTSTP to handle Ctrl-...");

                    if(command_count > 1 ){                 // Se houver mais de um comando em bg
                        sa.sa_handler = sigusr1_handler;    // Se receber SIGUSR1 envia SIGKILL para todos da mesma sessão (caso de exit)
                        
                        if (sigaction(SIGUSR1, &sa, NULL) == -1)
                            perror("Failed to set SIGUSR1 handler");
                    }
                    else{
                        signal(SIGUSR1, SIG_IGN);           // Se houver só um comando, ignora o sinal
                    }

                    
                }
                else if(pid > 0){
                    // sleep(1);
                    usleep(100);
                    sessions[current_session_index++] = getsid(pid);    // Guarda nova sessão atribuida ao processo intermediario no array
                }
                else{
                    perror("Erro ao criar o processo");
                    exit(1);
                }
            }

            sa.sa_handler = child_death_handler;

            if (sigaction(SIGCHLD, &sa, NULL) == -1)
                perror("Failed to set SIGCHLD to handle child death");

        
            for(int i = 0; i < command_count; i++){

                char* args[MAX_ARGS + 2];                       // Espaço adicional para o nome do programa, e terminação em NULL
                int arg_count = 0;                              // contador de argumentos

                args[arg_count++] = strtok(commands[i], " ");   // Pega o nome do programa

                /* Separar argumentos do comando */
                char* token;
                while ((token = strtok(NULL, " ")) != NULL) { 
                    args[arg_count] = token;
                    trim(args[arg_count]);

                    if( arg_count > MAX_ARGS && flag_foreground == 0 ){
                        if(pid > 0)
                            fprintf(stderr, "ERRO: O número máximo de argumentos é 3\n");
                        flag_input_valido = 0;
                        break;
                    }
                    else if( flag_foreground == 1 && arg_count > MAX_ARGS + 1 ){
                        if(pid > 0)
                            fprintf(stderr, "ERRO: O número máximo de argumentos é 3\n");
                        flag_input_valido = 0;
                        break;
                    }
                    arg_count++;
                }
                if( flag_foreground == 1 && arg_count > MAX_ARGS + 2 ){
                    if(pid > 0)
                        fprintf(stderr, "ERRO: O número máximo de argumentos é 3\n");
                    flag_input_valido = 0;
                }

                /* Terminar vetor de strings em NULL para passar pro exec*/
                if( flag_foreground == 1 ){
                    args[arg_count-1] = NULL;
                }
                else{
                    args[arg_count] = NULL;
                }


                if( flag_input_valido == 1 ){
                    // VERIFICAÇÃO DE OPERAÇÃO INTERNA - cd
                    if( strcmp(args[0], "cd") == 0){
                        if(pid > 0){
                            char buffer[256];
                            char *current_dir = getcwd(buffer, sizeof(buffer));             // Get diretorio atual
                            if (current_dir != NULL) {
                                if(args[1] != NULL ){
                                    if(strcmp(args[1], "..") == 0 ){                        // Se for operação de voltar um diretório
                                        if(chdir(dirname(current_dir)) != 0){               // chdir seta o diretório atual para o parametro passado
                                            printf("Falha ao retornar ao diretório pai!\n");    // |->> dirname retorna o diretório pai do parametro passado
                                        }
                                    }
                                    else{                                                   // Se não, tentar entrar no diretorio especificado
                                        // Concatenar o diretório desejado com o diretório atual
                                        char destination_dir[512];
                                        snprintf(destination_dir, sizeof(destination_dir), "%s/%s", current_dir, args[1]);
                                        
                                        //mudar de diretorio com o chdir
                                        if (chdir(destination_dir) != 0) {
                                            perror("Diretório não encontrado!\n");
                                        }
                                    }
                                }
                            } else {
                                printf("Falha ao obter o diretório atual.\n");
                            }
                            current_dir = getcwd(buffer, sizeof(buffer));           // Get diretorio atual
                            printf("Diretório atual: %s\n", current_dir);
                        }
                    }
                    /*Se não, tentar executar comando externo*/
                    else if (arg_count > 0){
                        if (pid == 0) {                               // Processo filho intermediario                                 
                            if(flag_foreground == 0){                 // Se em bg, mesmo session id do processo intermediario
                                
                                pid_t pid_bg = fork();
                                
                                if(pid_bg == 0){
                                    execvp(args[0], args);            // Chama o execvp para executar o comando com seus parametros
                                    perror("Erro ao executar o comando");
                                    exit(1);
                                }
                                else if(pid_bg > 0){                  // Processo intermediário
                                    //======= Registra um tratador para SIGCHLD =======//
                                    sa.sa_handler = child_death_handler;
                                    if (sigaction(SIGCHLD, &sa, NULL) == -1)
                                        perror("Failed to set SIGCHLD to handle child death");
                                }
                                else{
                                    perror("Erro ao criar o processo");
                                    exit(1);
                                }
                            }
                        } 
                        else if (pid > 0) { 
                            // printf("sid pai: %d", getsid(getpid()));               // Processo pai
                            if(command_count == 1 && flag_foreground == 1){           // Se há apenas um comando externo e foi terminado em %
                                pid_t pid_fg = fork();

                                if(pid_fg == 0){                                      // Processo filho
                                    execvp(args[0], args);                            // Chama o execvp para executar o comando com seus parametros
                                    perror("Erro ao executar o comando");
                                    exit(1);
                                } 
                                else if(pid_fg > 0){
                                    //===== Enquanto um processo roda em foreground ignorar sinais de ctrl+... =====//
                                    sa.sa_handler = ctrl_handler_fg;

                                    if ((sigaction(SIGINT, &sa, NULL) == -1) ||
                                        (sigaction(SIGQUIT, &sa, NULL) == -1) ||
                                        (sigaction(SIGTSTP, &sa, NULL) == -1))
                                            perror("Failed to set SIGINT || SIGQUIT || SIGTSTP to handle Ctrl-...");

                                    
                                    waitpid(pid_fg, NULL, 0);        // Espera pelo termino do processo em fg
                                    
                                    /* Ao terminar o processo em fg para de ignorar sinais de ctrl+...*/
                                    sa.sa_handler = ctrl_handler;

                                    if ((sigaction(SIGINT, &sa, NULL) == -1) ||
                                        (sigaction(SIGQUIT, &sa, NULL) == -1) ||
                                        (sigaction(SIGTSTP, &sa, NULL) == -1))
                                            perror("Failed to set SIGINT || SIGQUIT || SIGTSTP to handle Ctrl-...");
                                }
                                else{
                                    perror("Erro ao criar o processo");
                                    exit(1);
                                }
                            }
                            else{
                                sa.sa_handler = child_death_handler;
                                if (sigaction(SIGCHLD, &sa, NULL) == -1)
                                    perror("Failed to set SIGCHLD to handle child death");
                            }
                        }
                    }
                }
            }
            if(pid == 0){     // Filho intermediario espera pela morte dos filhos e retorna
                pid_t childpid;
                int status;
                while ((childpid = wait(&status)) > 0) {
                    if (WTERMSIG(status) == SIGUSR1) {                  // Se o filho terminou devido ao SIGUSR1
                        pid_t current_sid = getsid(0);                  // Get o id da sessão do processo
                        // Envia o sinal SIGKILL para todos os processos da sessão atual
                        if (current_sid != -1) {
                            if (kill(-current_sid, SIGKILL) != 0) {
                                perror("Erro ao enviar o sinal SIGKILL");
                                return 1;
                            }
                        } else {
                            perror("Erro ao obter o ID da sessão atual");
                            return 1;
                        }
                    }
                }
                removeSessionArray(sessions, getsid(0), current_session_index);
                return 0;
            }
        }
        /*========================== FIM (PROCESSAMENTO DOS COMANDOS) ============================*/
    }

    free(line_commands);
    for(int i = 0; i < MAX_EXTERNAL_COMMANDS; i++ ){
        free(commands[i]);
    }
    free(commands);

    return 0;
}


/*========================== TRIM FUNCTION =======================*/
static void trim(char *str) {
    int start = 0;
    int end = strlen(str) - 1;
    while (isspace(str[start])) {
        start++;
    }
    while (end > start && isspace(str[end])) {
        end--;
    }
    str[end + 1] = '\0';
    if (start > 0) {
        memmove(str, str + start, end - start + 2);
    }
}
/*======================== END (TRIM FUNCTION) ======================*/


/*========================== SIGCHLD handler ========================*/
void child_death_handler(int n){
    pid_t childpid;
    while (childpid = waitpid(-1, NULL, WNOHANG))
        if ((childpid == -1) && (errno != EINTR))
            break;
}

/*================ SIGINT, SIGQUIT, SIGTSTP handler =================*/
void ctrl_handler(int n){
    struct sigaction sa_hand;

    if ((sigemptyset(&sa_hand.sa_mask) == -1) ||
        (sigaddset(&sa_hand.sa_mask, SIGINT) == -1) ||
        (sigaddset(&sa_hand.sa_mask, SIGQUIT) == -1) ||
        (sigaddset(&sa_hand.sa_mask, SIGTSTP) == -1))
            perror("Failed to initialize the signal set");
    else if (sigprocmask(SIG_BLOCK, &sa_hand.sa_mask, NULL) == -1)
        perror("Failed to block SIGINT SIGQUIT and SIGTSTP");

    char handmsg[] = "Não adianta me enviar o sinal por Ctrl-... . Estou vacinado!\n";
    int msglen = sizeof(handmsg);
    write(STDERR_FILENO, handmsg, msglen);
}

/*====== SIGINT, SIGQUIT, SIGTSTP handler while process executing in fg =======*/
void ctrl_handler_fg(int n){
    
}

/*====== SIGUSR1 handler when executing more than one bg process =======*/
void sigusr1_handler(int n){
    pid_t current_sid = getsid(0); // Id da sessão do processo
    // Envia o sinal SIGKILL para todos os processos da sessão atual
    if (current_sid != -1) {
        if (kill(-current_sid, SIGKILL) != 0) {
            perror("Erro ao enviar o sinal SIGKILL");
        }
    } else {
        perror("Erro ao obter o ID da sessão atual");
    }
}

void removeSessionArray(pid_t* sessions, pid_t sid, int current_session_index){
    int i = 0;
    for(i = 0; i < current_session_index; i++){
        if(sessions[i] == sid){
            sessions[i] = 0;
            break;
        }
    }
}