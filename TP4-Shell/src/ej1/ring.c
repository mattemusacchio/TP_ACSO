#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// pipes: array de n pipes; cada pipes[i][0] es lectura de i→i+1
//          y pipes[i][1] es escritura de i→i+1
// p_p2c: pipe padre→hijo inicial
// p_c2p: pipe hijo inicial→padre


// función para cada hijo
void hijo(int i, int n, int start,
                  int pipes[][2],
                  int p_p2c[2], int p_c2p[2]) {
    int valor;
    int prev = (i - 1 + n) % n;

    // 1) Cerrar todos los extremos que NO usa este hijo
    for(int j = 0; j < n; j++) {
        // sólo leerá de pipes[prev][0] y escribirá en pipes[i][1]
        if (j == prev) {
            close(pipes[j][1]);
        } else if (j == i) {
            close(pipes[j][0]);
        } else {
            close(pipes[j][0]);
            close(pipes[j][1]);
        }
    }

    // 2) Si es el iniciador, recibe del padre y al final le devuelve
    if (i == start - 1) {
        // cierra extremos que no necesita
        close(p_p2c[1]);  // no escribe en p_p2c
        close(p_c2p[0]);  // no lee de p_c2p

        // a) leer valor inicial
        read(p_p2c[0], &valor, sizeof(valor));
        close(p_p2c[0]);

        // b) incrementar y pasar al siguiente
        valor++;
        write(pipes[i][1], &valor, sizeof(valor));
        close(pipes[i][1]);

        // c) esperar que recorra todo el anillo
        read(pipes[prev][0], &valor, sizeof(valor));
        close(pipes[prev][0]);

        // d) devolver resultado al padre
        write(p_c2p[1], &valor, sizeof(valor));
        close(p_c2p[1]);

    } else {
        // 3) Procesos intermedios (o el último): leen, incrementan y escriben
        read(pipes[prev][0], &valor, sizeof(valor));
        close(pipes[prev][0]);

        valor++;
        write(pipes[i][1], &valor, sizeof(valor));
        close(pipes[i][1]);
    }

    exit(0);
}

// función del padre
void padre(int n, int initial_value, int start,
                   int pipes[][2],
                   int p_p2c[2], int p_c2p[2]) {
    int status, resultado;

    // cerrar extremos que no usamos
    close(p_p2c[0]);   // no leemos de p_p2c
    close(p_c2p[1]);   // no escribimos en p_c2p

    // 1) enviar al hijo iniciador
    write(p_p2c[1], &initial_value, sizeof(initial_value));
    close(p_p2c[1]);

    // 2) esperar resultado
    read(p_c2p[0], &resultado, sizeof(resultado));
    close(p_c2p[0]);

    printf("Resultado final: %d\n", resultado);

    // 3) esperar a todos los hijos
    for(int i = 0; i < n; i++) {
        wait(&status);
    }
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <n> <c> <s>\n", argv[0]);
        exit(1);
    }

    int n     = atoi(argv[1]);
    int c     = atoi(argv[2]);
    int start = atoi(argv[3]);

    if (n < 3) {
        fprintf(stderr, "Error: se requieren al menos 3 procesos para el anillo\n");
        exit(1);
    }
    if (start < 1 || start > n) {
        fprintf(stderr, "Error: proceso inicial debe estar en [1..%d]\n", n);
        exit(1);
    }

    printf("Se crearán %d procesos, se enviará %d desde proceso %d\n",
           n, c, start);

    // 1) crear los n pipes del anillo
    int pipes[n][2];
    for (int i = 0; i < n; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }

    // 2) crear los dos pipes padre<->hijo inicial
    int p_p2c[2], p_c2p[2];
    if (pipe(p_p2c) == -1 || pipe(p_c2p) == -1) {
        perror("pipe padre-hijo");
        exit(1);
    }

    // 3) fork de los n hijos
    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        if (pid == 0) {
            hijo(i, n, start, pipes, p_p2c, p_c2p);
        }
    }

    // 4) código del padre: lanzar la comunicación y recoger el resultado
    padre(n, c, start, pipes, p_p2c, p_c2p);
    return 0;
}
