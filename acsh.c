#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1000
#define MAX_COMMAND_LENGTH 200
#define MAX_COMMANDS 5
#define MAX_ARGS 3


pid_t foreground_pid = 0;
pid_t session_pid = 0;

static void trim(char *str);

void process_command(char* command);

void execute_command(char* command);

int main( int argc, char* argv[]){
    char* line_commands = calloc(MAX_LINE_LENGTH, sizeof(char));    // Comando de entrada do usuario no shell
    char** commands = calloc(MAX_COMMANDS, sizeof(char*));          // Vetor de comandos ( no maximo MAX_COMMANDS)
    for(int i = 0; i < MAX_COMMANDS; i++ ){
        commands[i] = calloc(MAX_COMMAND_LENGTH, sizeof(char));     // Comandos em si
    }
    int command_count = 0;                                          // Numero de comandos atual inserido pelo usuario
    const char delimiter[] = "<3";                                  // Delimitador usado

    while (1){

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
            if (session_pid != 0) {                     //TO-DO: COMENTAR ESSE TRECHO DE CODIGO
                kill(-session_pid, SIGUSR1);
                session_pid = 0;
            }
            break;
        }

        char *token;
        char *remaining = line_commands;
        command_count = 0;

        /* função strstr() é usada para localizar a próxima ocorrência do delimitador na string*/
        while ((token = strstr(remaining, delimiter)) != NULL) {
            
            /*Numero de comandos é maior que o permitido*/
            if( command_count+2 > MAX_COMMANDS ){
                fprintf(stderr, "ERRO: Numero de comandos é maior que o permitido\n");
                exit(-1);
            }

            /*Em seguida, é calculado o comprimento da substring antes do delimitador*/
            int length = token - remaining;

            /*A função strncpy() é usada para copiar essa substring de tamanho length e salvar comando na string de comandos*/
            strncpy(commands[command_count], remaining, length);
            
            /*Remover espacos em branco no comeco e no final*/
            trim(commands[command_count]);

            /*Incrementar numero de comandos atual*/
            command_count++;
            
            /*Por fim, a variável remaining é atualizada para apontar para o próximo caractere após o delimitador*/
            remaining = token + strlen(delimiter);
        }
        
        /*Salvar ultimo comando na string de comandos*/
        strcpy(commands[command_count], remaining);

        /*Remover espacos em branco no comeco e no final*/
        trim(commands[command_count]);

        /*Incrementar numero de comandos atual*/
        command_count++;
        /*============= FIM (LEITURA DO COMANDO DE ENTRADA E SEPARACAO DOS COMANDOS) =============*/


        /*============================= PROCESSAMENTO DOS COMANDOS ===============================*/
        for(int i = 0; i < command_count; i++){
            process_command(commands[i]);
        }
        /*========================== FIM (PROCESSAMENTO DOS COMANDOS) ============================*/

        printf("========== FIM WHILE========\n");

    }

    free(line_commands);
    for(int i = 0; i < MAX_COMMANDS; i++ ){
        free(commands[i]);
    }
    free(commands);

    return 0;
}

/*
Input Test:
command1 <3 command2 arg1 <3 command3 arg1 arg2 <3 command4
*/


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

void process_command(char* command) {
    printf("Processando comando '%s' ...\n", command);

    pid_t pid = fork();

    if (pid == 0) { // Processo filho
        if (session_pid != 0) {
            setpgid(0, session_pid);  // Seta process group ID = session PID
        }

        printf("Eu sou o filho %d e vou executar o comando '%s' ...\n", getpid(), command);

        execute_command(command);
        exit(0);
    } 
    else if (pid > 0) { // Processo pai
        printf("Eu sou o pai, vulgo %d\n", getpid());

        if (session_pid == 0) {
            session_pid = pid;
        }

        printf("Aguardando meu filho %d terminar\n", pid);

        wait(NULL);
        printf("\nMeu filho %d terminou!!!\n", pid);
    }
    else {
        perror("Erro ao criar o processo");
        exit(1);
    }
}

void execute_command(char* command) {
    char* args[MAX_ARGS + 2];  // Espaço adicional para o nome do programa e terminação em NULL
    int arg_count = 0;

    args[arg_count++] = strtok(command, " ");  // Pega o nome do programa

    // Separar argumentos do comando
    char* token;
    while ((token = strtok(NULL, " ")) != NULL && arg_count <= MAX_ARGS + 1) { 
        args[arg_count] = token;
        trim(args[arg_count]);
        arg_count++;
    }
    args[arg_count] = NULL;

    for(int i = 0; i <= arg_count; i++ ){
        printf("args[%d] = '%s'\n", i, args[i]);
    }
    printf("\n");

    // Tentar executar o comando
    if (arg_count > 0) {
        printf("%s:\n", command);
        execvp(args[0], args);
        perror("Erro ao executar o comando");
        exit(1);
    }
}