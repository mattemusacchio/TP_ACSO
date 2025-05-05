#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/**
 * TODO
 */
int directory_findname(struct unixfilesystem *fs, const char *name, int dirinumber, struct direntv6 *dirEnt) {
    struct inode dirinode;

    if (inode_iget(fs, dirinumber, &dirinode) < 0) {
        return -1;
    }

    int filesize = (dirinode.i_size0 << 16) | dirinode.i_size1;
    int num_blocks = (filesize + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;

    char block[DISKIMG_SECTOR_SIZE];

    for (int b = 0; b < num_blocks; b++) {
        int nbytes = file_getblock(fs, dirinumber, b, block);
        if (nbytes < 0) {
            return -1;
        }

        int num_entries = nbytes / sizeof(struct direntv6);
        struct direntv6 *entries = (struct direntv6 *)block;

        for (int i = 0; i < num_entries; i++) {
            if (strncmp(entries[i].d_name, name, FILENAME_MAX) == 0) {
                *dirEnt = entries[i];
                return 0;
            }
        }
    }

    return -1;
}
