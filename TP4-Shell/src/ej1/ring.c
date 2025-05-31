#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// Comunicación básica entre procesos  
// Como warm up para este primer ejercicio, el objetivo es implementar un esquema 
// de comunicación en forma de anillo para interconectar los procesos. En un esquema 
// de anillo se da con al menos tres procesos están conectados formando un bucle 
// cerrado. Cada proceso está comunicado exactamente con dos procesos: su 
// predecesor y su sucesor. Recibe un mensaje del predecesor y lo envía al sucesor. 
// En este caso, la comunicación se llevará a cabo a través de pipes, las cuales deben 
// ser implementadas. 
 
 
// Al inicio, alguno de los procesos del anillo recibirá del padre un número entero 
// como mensaje a transmitir. Este mensaje será enviado al siguiente proceso en el 
// anillo, quien, tras recibirlo, lo incrementará en uno y luego lo enviará al siguiente 
// proceso en el anillo. Este proceso continuará hasta que el proceso que inició la 
// comunicación reciba, del último proceso, el resultado del mensaje inicialmente 
// enviado. 
 
 
// Se sugiere que el programa inicial cree un conjunto de procesos hijos, que deben 
// ser organizados para formar un anillo. Por ejemplo, el hijo 1 recibe, del padre, el 
// mensaje, lo incrementa y lo envía al hijo 2. Este último lo incrementa nuevamente y 
// lo pasa al hijo 3, y así sucesivamente, hasta llegar al último hijo, que incrementa el 
// valor por última vez y lo envía de vuelta al proceso padre. Este último debe mostrar 
// el resultado final del proceso de comunicación en la salida estándar. 
 
 
// Se espera que el programa pueda ejecutarse como:  
 
// ./anillo <n><c><s>,  
 
// donde: 
 
// <n> es la cantidad de procesos hijos del anillo. 
// <c> es el valor del mensaje inicial. 
// <s> es el número de proceso que inicia la comunicación 
 
// Les proveemos un archivo ring.c deberían trabajar sobre ese archivo pero 
// sientanse libres de modificarlo e incluso hacer sus propias librerías.  
 
 
// IMPORTANTE: para que les sirva como referencia la solución del problema 
// programada por nosotros tiene del orden de 40 líneas de código.

void funcion_hijo(int pipes[2][2], int posicion, int n, int start, int pipe_padre[2]) {
    int valor;
    
    // Cerrar los extremos no utilizados de los pipes
    if(posicion == 0) {
        close(pipes[0][1]);
        close(pipes[1][0]);
    } else if(posicion == n-1) {
        close(pipes[0][0]);
        close(pipes[1][1]);
    } else {
        close(pipes[0][1]);
        close(pipes[1][0]);
    }

    // Cerrar extremo de escritura del pipe con el padre si no es el proceso inicial
    if(posicion != start - 1) {
        close(pipe_padre[1]);
    }
    close(pipe_padre[0]); // Todos cierran la lectura del pipe con el padre

    // Si este es el proceso que debe iniciar la comunicación
    if(posicion == start - 1) {
        // Leer el valor inicial del padre
        read(pipe_padre[0], &valor, sizeof(int));
        
        // Incrementar el valor
        valor++;
        
        // Enviar al siguiente proceso
        write(pipes[1][1], &valor, sizeof(int));
        
        // Esperar a que el valor complete el ciclo
        if(posicion == 0) {
            read(pipes[0][0], &valor, sizeof(int));
        } else {
            read(pipes[1][0], &valor, sizeof(int));
        }
        
        // Enviar resultado final al padre
        write(pipe_padre[1], &valor, sizeof(int));
    } else {
        // Leer el valor del pipe anterior
        if(posicion == 0) {
            read(pipes[0][0], &valor, sizeof(int));
        } else {
            read(pipes[1][0], &valor, sizeof(int));
        }

        // Incrementar el valor
        valor++;

        // Escribir el valor en el siguiente pipe
        if(posicion == n-1) {
            write(pipes[0][1], &valor, sizeof(int));
        } else {
            write(pipes[1][1], &valor, sizeof(int));
        }
    }

    exit(0);
}

void funcion_padre(int pipes[2][2], int buffer[1], int n, int start, int pipe_padre[2]) {
    int status;
    
    // Cerrar extremo de lectura del pipe con los hijos
    close(pipe_padre[1]);
    
    // Enviar el valor inicial al proceso que debe comenzar
    write(pipe_padre[0], buffer, sizeof(int));

    // Esperar el resultado final del proceso inicial
    read(pipe_padre[1], buffer, sizeof(int));
    printf("Resultado final: %d\n", buffer[0]);

    // Esperar a que todos los procesos hijos terminen
    for(int i = 0; i < n; i++) {
        wait(&status);
    }
}

int main(int argc, char **argv)
{	
	int start, pid, n;
	int buffer[1];
	int pipes[2][2];  // Array para almacenar los pipes
    int pipe_padre[2]; // Pipe para comunicación con el proceso inicial

	if (argc != 4){ printf("Uso: anillo <n> <c> <s> \n"); exit(0);}
    
    /* Parsing of arguments */
    n = atoi(argv[1]);
    buffer[0] = atoi(argv[2]);
    start = atoi(argv[3]);
    
    if(n < 3) {
        printf("Error: se requieren al menos 3 procesos para formar el anillo\n");
        exit(1);
    }
    
    if(start < 1 || start > n) {
        printf("Error: el proceso inicial debe estar entre 1 y %d\n", n);
        exit(1);
    }
    
    printf("Se crearán %i procesos, se enviará el valor %i desde proceso %i \n", n, buffer[0], start);
    
    // Crear los pipes necesarios
    for(int i = 0; i < 2; i++) {
        if(pipe(pipes[i]) == -1) {
            perror("Error al crear pipe");
            exit(1);
        }
    }
    
    // Crear pipe para comunicación con el proceso inicial
    if(pipe(pipe_padre) == -1) {
        perror("Error al crear pipe");
        exit(1);
    }

    // Crear los procesos hijos
    for(int i = 0; i < n; i++) {
        pid = fork();
        if(pid == -1) {
            perror("Error al crear proceso");
            exit(1);
        }
        if(pid == 0) {  // Proceso hijo
            funcion_hijo(pipes, i, n, start, pipe_padre);
        }
    }

    // Proceso padre
    funcion_padre(pipes, buffer, n, start, pipe_padre);

    return 0;
}
