
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/**
 * TODO
 */
int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    if (pathname[0] != '/') {
        // Solo se permiten paths absolutos
        return -1;
    }

    int curr_inumber = 1;  // El inodo raíz siempre es 1
    char path_copy[strlen(pathname) + 1];
    strcpy(path_copy, pathname);

    // Tokenizamos usando "/"
    char *token = strtok(path_copy, "/");

    while (token != NULL) {
        struct direntv6 entry;

        if (directory_findname(fs, token, curr_inumber, &entry) < 0) {
            return -1;  // No se encontró el componente del path
        }

        curr_inumber = entry.d_inumber;
        token = strtok(NULL, "/");
    }

    return curr_inumber;
}

