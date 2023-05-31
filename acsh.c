#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

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


#define MAX_COMMANDS_LENGTH 1000
#define MAX_COMMAND_LENGTH 100
#define MAX_COMMANDS 5
#define MAX_ARGS 3

int main( int argc, char* argv[]){

    char* line_commands = calloc(MAX_COMMANDS_LENGTH, sizeof(char));
    char** commands = calloc(MAX_COMMAND_LENGTH, sizeof(char*));
    for(int i = 0; i < MAX_COMMANDS; i++ ){
        commands[i] = calloc(MAX_COMMAND_LENGTH, sizeof(char));
    }
    char* args[MAX_ARGS];
    const char delimiter[] = "<3";


    int num_commands = 0; // Numero de comandos atual inserido pelo usuario

    while (1){

        // Exibicao do prompt
        printf("acsh> ");

        /*=============== LEITURA DO COMANDO DE ENTRADA E SEPARACAO DOS COMANDOS ===============*/

        // Aguarda o comando do usuario e le ele quando inserido 
        fgets(line_commands, MAX_COMMANDS_LENGTH, stdin);

        // Removendo o caractere nova linha ('\n') do final do comando e substitui por \0
        line_commands[strcspn(line_commands, "\n")] = '\0';

        char *token;
        char *remaining = line_commands;
        num_commands = 0;

        /* função strstr() é usada para localizar a próxima ocorrência do delimitador na string*/
        while ((token = strstr(remaining, delimiter)) != NULL) {
            
            /*Numero de comandos é maior que o permitido*/
            if( num_commands+2 > MAX_COMMANDS ){
                fprintf(stderr, "ERRO: Numero de comandos é maior que o permitido\n");
                exit(-1);
            }

            /*Em seguida, é calculado o comprimento da substring antes do delimitador*/
            int length = token - remaining;
            char substring[length + 1];

            /*A função strncpy() é usada para copiar essa substring em uma variável temporária*/
            strncpy(substring, remaining, length);
            substring[length] = '\0';
            
            /*Salvar comando na string de comandos*/
            strcpy(commands[num_commands], substring);

            /*Remover espacos em branco no comeco e no final*/
            trim(commands[num_commands]);

            /*Incrementar numero de comandos atual*/
            num_commands++;
            
            /*Por fim, a variável remaining é atualizada para apontar para o próximo caractere após o delimitador*/
            remaining = token + strlen(delimiter);
        }
        
        /*Salvar ultimo comando na string de comandos*/
        strcpy(commands[num_commands], remaining);

        /*Remover espacos em branco no comeco e no final*/
        trim(commands[num_commands]);

        /*Incrementar numero de comandos atual*/
        num_commands++;

        /*============= FIM (LEITURA DO COMANDO DE ENTRADA E SEPARACAO DOS COMANDOS) =============*/

        for(int i = 0; i < num_commands; i++){
            printf("Command: '%s'\n", commands[i]);
        }
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