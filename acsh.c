//#define _POSIX_SOURCE

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

static void trim(char *str);

void child_death_handler(int n);
void ctrl_handler(int n);

// void process_command(char* command);

// void execute_command(char* command);

int main( int argc, char* argv[]){
    char* line_commands = calloc(MAX_LINE_LENGTH, sizeof(char));            // Comando de entrada do usuario no shell
    char** commands = calloc(MAX_EXTERNAL_COMMANDS, sizeof(char*));         // Vetor de comandos ( no maximo MAX_COMMANDS)
    for(int i = 0; i < MAX_EXTERNAL_COMMANDS; i++ ){
        commands[i] = calloc(MAX_COMMAND_LENGTH, sizeof(char));             // Comandos em si
    }
    const char delimiter[] = "<3";                                          // Delimitador usado

    /* struct sigaction sa;
    sa.sa_handler = ctrl_handler;
    sa.sa_flags = 0;

    if ((sigemptyset(&sa.sa_mask) == -1) ||
       (sigaction(SIGINT, &sa, NULL) == -1) ||
       (sigaction(SIGQUIT, &sa, NULL) == -1) ||
       (sigaction(SIGTSTP, &sa, NULL) == -1))
            perror("Failed to set SIGINT || SIGQUIT || SIGTSTP to handle Ctrl-..."); */

    while (1){
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
            // if (session_pid != 0) {                     //TO-DO: COMPLEMENTAR COMENTAR ESSE TRECHO DE CODIGO
            //     kill(-session_pid, SIGUSR1);
            //     session_pid = 0;
            // }
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


            // printf("Comando\n%s\n", commands[command_count]);


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


        /*============================= PROCESSAMENTO DOS COMANDOS ===============================*/
        if( flag_input_valido == 1 ){
            for(int i = 0; i < command_count; i++){

                flag_foreground = 0;
                /*Verificação de foreground*/
                if(commands[i][strlen(commands[i])-1] == FOREGROUND){
                    flag_foreground = 1;
                    if(internal_command_count == 1){
                        fprintf(stderr, "WARNING: Comandos internos já são executados em foreground!\nContinuando a execução...\n");
                    }
                    if(external_command_count > 1){
                        fprintf(stderr, "ERRO: Não pode é possível rodar mais de um programa em foreground\n");
                        flag_input_valido = 0;
                        break;
                    }
                }


                /*======================== DEBUG CODE ========================*/
                printf("Processando comando '%s' ...\n", commands[i]);
                printf("Em %d\n", flag_foreground);
                /*====================== END DEBUG CODE ======================*/


                char* args[MAX_ARGS + 2];                                   // Espaço adicional para o nome do programa, e terminação em NULL
                int arg_count = 0;

                args[arg_count++] = strtok(commands[i], " ");               // Pega o nome do programa

                /* Separar argumentos do comando */
                char* token;
                while ((token = strtok(NULL, " ")) != NULL) { 
                    args[arg_count] = token;
                    trim(args[arg_count]);

                    if( arg_count > MAX_ARGS && flag_foreground == 0 ){
                        fprintf(stderr, "ERRO: O número máximo de argumentos é 3\n");
                        flag_input_valido = 0;
                        break;
                    }
                    else if( flag_foreground == 1 && arg_count > MAX_ARGS + 1 ){
                        fprintf(stderr, "ERRO: O número máximo de argumentos é 3\n");
                        flag_input_valido = 0;
                        break;
                    }
                    arg_count++;
                }
                if( flag_foreground == 1 && arg_count > MAX_ARGS + 2 ){
                    fprintf(stderr, "ERRO: O número máximo de argumentos é 3\n");
                    flag_input_valido = 0;
                }

                if( flag_foreground == 1 ){
                    args[arg_count-1] = NULL;
                }
                else{
                    args[arg_count] = NULL;
                }


                if( flag_input_valido == 1 ){
                    // // VERIFICAÇÃO DE OPERAÇÃO INTERNA - cd
                    // if( strcmp(args[0], "cd") == 0){
                    //     char buffer[256];
                    //     char *current_dir = getcwd(buffer, sizeof(buffer));             // Get diretorio atual
                    //     if (current_dir != NULL) {
                    //         if(args[1] != NULL ){
                    //             if(strcmp(args[1], "..") == 0 ){                        // Se for operação de voltar um diretório
                    //                 if(chdir(dirname(current_dir)) != 0){               // chdir seta o diretório atual para o parametro passado
                    //                     printf("Falha ao retornar ao diretório pai!\n");    // |->> dirname retorna o diretório pai do parametro passado
                    //                 }
                    //             }
                    //             else{                                                   // Se não, tentar entrar no diretorio especificado
                    //                 // Concatenar o diretório desejado com o diretório atual
                    //                 char destination_dir[512];
                    //                 snprintf(destination_dir, sizeof(destination_dir), "%s/%s", current_dir, args[1]);
                                    
                    //                 //mudar de diretorio com o chdir
                    //                 if (chdir(destination_dir) != 0) {
                    //                     perror("Diretório não encontrado!\n");
                    //                 }
                    //             }
                    //         }
                    //     } else {
                    //         printf("Falha ao obter o diretório atual.\n");
                    //     }
                    //     current_dir = getcwd(buffer, sizeof(buffer));           // Get diretorio atual
                    //     printf("Diretório atual: %s\n", current_dir);
                    // }
                    /*Se não, tentar executar comando externo*/
                    // else if (arg_count > 0){

                    //     pid_t pid = fork();                                                 // fork

                    //     if (pid == 0) {                                                     // Processo filho
                    //         printf("sid child antes: %d", getsid(getpid()));
                    //         if (setsid() < 0) {                                             // Muda o session id do processo filho
                    //             fprintf(stderr, "Falha ao iniciar uma nova sessão para o processo filho.\n");
                    //             exit(1);
                    //         }
                    //         printf("sid child depois: %d", getsid(getpid()));
                            
                    //         execvp(args[0], args);                                          // Chama o execvp para executar o comando com seus parametros
                    //         perror("Erro ao executar o comando");
                    //         exit(1);
                    //     } 
                    //     else if (pid > 0) {
                    //         printf("sid pai: %d", getsid(getpid()));                        // Processo pai
                    //         if(command_count == 1 && flag_foreground == 1){                 // Se há apenas um comando externo e foi terminado em %
                    //             //===== Enquanto um processo roda em foreground ignorar sinais de ctrl+... =====//
                    //             struct sigaction sa_fg;
                    //             sa_fg.sa_handler = ctrl_handler;
                    //             sa_fg.sa_flags = 0;

                    //             if ((sigemptyset(&sa_fg.sa_mask) == -1) ||
                    //                 (sigaddset(&sa_fg.sa_mask, SIGINT) == -1) ||
                    //                 (sigaddset(&sa_fg.sa_mask, SIGQUIT) == -1) ||
                    //                 (sigaddset(&sa_fg.sa_mask, SIGTSTP) == -1))
                    //                     perror("Failed to initialize the signal set");
                    //             else if (sigprocmask(SIG_BLOCK, &sa_fg.sa_mask, NULL) == -1)
                    //                 perror("Failed to block SIGINT SIGQUIT and SIGTSTP");

                                
                    //             waitpid(pid, NULL, 0);                                      // Espera pelo termino do processo em fg
                    //         }
                    //         else{
                    //             signal(SIGCHLD, child_death_handler);                       // Registra um handler para SIGCHLD
                    //         }
                    //     }
                    //     else {
                    //         perror("Erro ao criar o processo");
                    //         exit(1);
                    //     }
                    // }
                }
            }
        }
        
        /*========================== FIM (PROCESSAMENTO DOS COMANDOS) ============================*/

        /*======================== DEBUG CODE ========================*/
        // printf("========== FIM WHILE========\n");
        /*====================== END DEBUG CODE ======================*/
        
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
    /* struct sigaction sa_hand;

    if ((sigemptyset(&sa_hand.sa_mask) == -1) ||
        (sigaddset(&sa_hand.sa_mask, SIGINT) == -1) ||
        (sigaddset(&sa_hand.sa_mask, SIGQUIT) == -1) ||
        (sigaddset(&sa_hand.sa_mask, SIGTSTP) == -1))
            perror("Failed to initialize the signal set");
    else if (sigprocmask(SIG_BLOCK, &sa_hand.sa_mask, NULL) == -1)
        perror("Failed to block SIGINT SIGQUIT and SIGTSTP");


    char handmsg[] = "Não adianta me enviar o sinal por Ctrl-... . Estou vacinado!\n";
    int msglen = sizeof(handmsg);
    write(STDERR_FILENO, handmsg, msglen); */
}


// void process_command(char* command) {

//     /*======================== DEBUG CODE ========================*/
//     // printf("Processando comando '%s' ...\n", command);
//     /*====================== END DEBUG CODE ======================*/
    
//     pid_t pid = fork();

//     if (pid == 0) {                                         // Processo filho

//         /*======================== DEBUG CODE ========================*/
//         // printf("Eu sou o filho %d e vou executar o comando '%s' ...\n", getpid(), command);
//         /*====================== END DEBUG CODE ======================*/
        
//         execute_command(command);                           // Chama a funcao que separa os argumentos e executa o comando finalmente
//         exit(0);
//     } 
//     else if (pid > 0) {                                     // Processo pai

//         /*======================== DEBUG CODE ========================*/
//         // printf("Eu sou o pai, vulgo %d\n", getpid());
//         // printf("Aguardando meu filho %d terminar\n", pid);
//         /*====================== END DEBUG CODE ======================*/

//         wait(NULL);                                         // Aguarda o termino do processo filho

//         /*======================== DEBUG CODE ========================*/
//         // printf("\nMeu filho %d terminou!!!\n", pid);
//         /*====================== END DEBUG CODE ======================*/
        
//     }
//     else {
//         perror("Erro ao criar o processo");
//         exit(1);
//     }
// }

// void execute_command(char* command) {
//     char* args[MAX_ARGS + 2];                               // Espaço adicional para o nome do programa e terminação em NULL
//     int arg_count = 0;

//     args[arg_count++] = strtok(command, " ");               // Pega o nome do programa

//     /* Separar argumentos do comando */
//     char* token;
//     while ((token = strtok(NULL, " ")) != NULL && arg_count <= MAX_ARGS + 1) { 
//         args[arg_count] = token;
//         trim(args[arg_count]);
//         arg_count++;
//     }
//     args[arg_count] = NULL;                                 // Ultimo argumento do execvp deve ser NULL


//     /*======================== DEBUG CODE ========================*/
//     // for(int i = 0; i <= arg_count; i++ ){
//     //     printf("args[%d] = '%s'\n", i, args[i]);
//     // }
//     // printf("\n");
//     /*====================== END DEBUG CODE ======================*/

//     /*Tentar executar o comando*/
//     if (arg_count > 0) {
//         /*======================== DEBUG CODE ========================*/
//         // printf("%s:\n", command);
//         /*====================== END DEBUG CODE ======================*/
        
//         execvp(args[0], args);                              // Chama o execvp para executar o comando com seus parametros
//         perror("Erro ao executar o comando");
//         exit(1);
//     }
// }