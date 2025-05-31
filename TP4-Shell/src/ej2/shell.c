#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_COMMANDS 200
#define MAX_ARGS 64

// Función para verificar si un pipe es válido
int es_pipe_valido(const char *cmd) {
    int in_quotes = 0;
    char quote_char = 0;
    
    // Verificar pipe al inicio
    if (cmd[0] == '|') return 0;
    
    // Verificar pipe al final
    if (cmd[strlen(cmd) - 1] == '|') return 0;
    
    // Verificar pipes consecutivos, ignorando pipes dentro de comillas
    for (int i = 0; cmd[i] != '\0'; i++) {
        if ((cmd[i] == '"' || cmd[i] == '\'') && (!in_quotes || quote_char == cmd[i])) {
            if (in_quotes) {
                in_quotes = 0;
                quote_char = 0;
            } else {
                in_quotes = 1;
                quote_char = cmd[i];
            }
            continue;
        }
        
        if (!in_quotes && cmd[i] == '|' && cmd[i + 1] == '|') return 0;
    }
    
    return 1;
}

// Función para dividir el comando en pipes
int split_pipes(char *command, char **commands) {
    int count = 0;
    int in_quotes = 0;
    char quote_char = 0;
    char *start = command;
    
    for (char *p = command; *p; p++) {
        if ((*p == '"' || *p == '\'') && (!in_quotes || quote_char == *p)) {
            if (in_quotes) {
                in_quotes = 0;
                quote_char = 0;
            } else {
                in_quotes = 1;
                quote_char = *p;
            }
            continue;
        }
        
        if (!in_quotes && *p == '|') {
            *p = '\0';
            commands[count++] = strdup(start);
            start = p + 1;
            while (*(start) == ' ') start++;
        }
    }
    
    if (*start) {
        commands[count++] = strdup(start);
    }
    
    return count;
}

char* get_next_token(char **str) {
    if (!str || !*str) return NULL;

    // Saltar espacios iniciales
    while (**str == ' ' || **str == '\t') (*str)++;

    if (**str == '\0') return NULL;

    char *start = *str;
    char *end;
    char quote_char = 0;

    // Si empieza con comillas
    if (*start == '"' || *start == '\'') {
        quote_char = *start;
        start++;             // Saltar la comilla inicial
        end = start;

        while (*end && *end != quote_char) end++;

        if (*end == quote_char) {
            *end = '\0';
            *str = end + 1;
        } else {
            // Comilla sin cerrar, devolver hasta fin de string
            *str = end;
        }

        return strdup(start); // incluso si está vacío
    }

    // No empieza con comillas, leer hasta espacio
    end = start;
    while (*end && *end != ' ' && *end != '\t') end++;

    if (*end != '\0') {
        *end = '\0';
        *str = end + 1;
    } else {
        *str = end;
    }

    return strdup(start);
}



// Función para contar argumentos considerando comillas
int contar_argumentos(const char *command) {
    char *cmd = strdup(command);
    if (!cmd) {
        perror("Error en strdup");
        exit(1);
    }
    
    int count = 0;
    int in_quotes = 0;
    char quote_char = 0;
    int in_arg = 0;
    
    for (int i = 0; cmd[i] != '\0'; i++) {
        // Manejar comillas
        if ((cmd[i] == '"' || cmd[i] == '\'') && (!in_quotes || quote_char == cmd[i])) {
            if (in_quotes) {
                in_quotes = 0;
                quote_char = 0;
            } else {
                if (!in_arg) count++;
                in_quotes = 1;
                quote_char = cmd[i];
                in_arg = 1;
            }
            continue;
        }
        
        // Contar argumentos fuera de comillas
        if (!in_quotes) {
            if (cmd[i] == ' ' || cmd[i] == '\t') {
                in_arg = 0;
            } else if (!in_arg) {
                count++;
                in_arg = 1;
            }
        }
    }
    
    free(cmd);
    return count;
}

int comillas_balanceadas(const char *cmd) {
    int in_quotes = 0;
    char quote_char = 0;
    for (int i = 0; cmd[i] != '\0'; i++) {
        if ((cmd[i] == '"' || cmd[i] == '\'') && (!in_quotes || quote_char == cmd[i])) {
            if (in_quotes) {
                in_quotes = 0;
                quote_char = 0;
            } else {
                in_quotes = 1;
                quote_char = cmd[i];
            }
        }
    }
    return !in_quotes;
}


int parse_command(char *command, char **args) {
    // Primero verificar el número de argumentos
    int num_args = contar_argumentos(command);
    if (num_args > MAX_ARGS) {
        fprintf(stderr, "Error: demasiados argumentos (máximo %d)\n", MAX_ARGS);
        return -1;
    }
    
    int i = 0;
    char *cmd = strdup(command);
    if (!cmd) {
        perror("Error en strdup");
        exit(1);
    }
    
    char *current = cmd;
    char *token;
    
    while ((token = get_next_token(&current)) != NULL && i < MAX_ARGS) {
        args[i] = strdup(token);
        if (!args[i]) {
            perror("Error en strdup");
            free(cmd);
            for (int j = 0; j < i; j++) {
                free(args[j]);
            }
            exit(1);
        }
        i++;
    }
    
    args[i] = NULL;
    free(cmd);
    return i;
}

// Función para liberar la memoria de los argumentos
void free_args(char **args) {
    if (!args) return;
    for(int i = 0; args[i] != NULL; i++) {
        free(args[i]);
    }
}

// Función que ejecuta el proceso hijo
void hijo(int pipes[][2], int pos, int total_commands, char *command) {
    // Configurar entrada desde pipe anterior (excepto primer comando)
    if (pos > 0) {
        if (dup2(pipes[pos-1][0], STDIN_FILENO) == -1) {
            perror("Error en dup2");
            exit(1);
        }
    }
    
    // Configurar salida hacia siguiente pipe (excepto último comando)
    if (pos < total_commands - 1) {
        if (dup2(pipes[pos][1], STDOUT_FILENO) == -1) {
            perror("Error en dup2");
            exit(1);
        }
    }
    if (pos > 0) close(pipes[pos - 1][0]);
    if (pos < total_commands - 1) close(pipes[pos][1]);

    
    // Cerrar todos los pipes en el hijo
    for (int j = 0; j < total_commands - 1; j++) {
        if (j != pos - 1) close(pipes[j][0]);
        if (j != pos) close(pipes[j][1]);
    }
    
    // Separar comando en argumentos y ejecutar
    char *args[MAX_ARGS];
    if (parse_command(command, args) == -1) {
        free_args(args);
        exit(1);
    }
    
    if (args[0] == NULL) {
        fprintf(stderr, "Error: comando vacío\n");
        free_args(args);
        exit(1);
    }

    // Deshabilitar buffering para evitar problemas con grep
    setbuf(stdout, NULL);
    
    execvp(args[0], args);
    
    // Si llegamos aquí, hubo un error
    perror("Error ejecutando comando");
    free_args(args);
    exit(1);
}

// Función que ejecuta el proceso padre
void padre(int pipes[][2], int command_count, char *commands[]) {
    pid_t *pids = malloc(command_count * sizeof(pid_t));
    if (!pids) {
        perror("Error en malloc");
        exit(1);
    }
    
    // Crear los procesos hijos
    for (int i = 0; i < command_count; i++) {
        pids[i] = fork();
        
        if (pids[i] == -1) {
            perror("Error en fork");
            free(pids);
            exit(1);
        }
        
        if (pids[i] == 0) {  // Proceso hijo
            free(pids);
            hijo(pipes, i, command_count, commands[i]);
            exit(1);
        }
    }
    
    // Cerrar todos los pipes en el padre después de crear todos los hijos
    for (int i = 0; i < command_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Esperar a que terminen todos los hijos en orden
    int status;
    for (int i = 0; i < command_count; i++) {
        waitpid(pids[i], &status, 0);
    }
    
    free(pids);
    
    // Liberar memoria de los comandos
    for (int i = 0; i < command_count; i++) {
        free(commands[i]);
    }
}

int main() {
    char command[256];
    char *commands[MAX_COMMANDS];
    int command_count = 0;

    while (1) 
    {
        if (isatty(STDIN_FILENO)) {
            printf("Shell> ");
            fflush(stdout);
        }

        
        if (fgets(command, sizeof(command), stdin) == NULL) {
            printf("\n");
            break;
        }
        
        command[strcspn(command, "\n")] = '\0';
        
        // Salir si el comando es "exit"
        if (strcmp(command, "exit") == 0) {
            break;
        }
        
        // Si la línea está vacía, continuar
        if (strlen(command) == 0) {
            continue;
        }
        
        // Verificar si el comando tiene pipes válidos
        if (!es_pipe_valido(command)) {
            fprintf(stderr, "Error: sintaxis de pipe inválida\n");
            continue;
        }

        if (!comillas_balanceadas(command)) {
            fprintf(stderr, "Error: comillas no balanceadas\n");
            continue;
        }

        // Dividir el comando en pipes
        command_count = split_pipes(command, commands);
        
        if (command_count == 0) continue;

        // Crear los pipes necesarios
        int pipes[MAX_COMMANDS-1][2];
        for (int i = 0; i < command_count - 1; i++) {
            if (pipe(pipes[i]) == -1) {
                perror("Error creando pipe");
                for (int j = 0; j < command_count; j++) {
                    free(commands[j]);
                }
                exit(1);
            }
        }

        padre(pipes, command_count, commands);
    }
    return 0;
}

