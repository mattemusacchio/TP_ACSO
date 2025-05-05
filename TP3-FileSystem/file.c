#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "file.h"
#include "inode.h"
#include "diskimg.h"

/**
 * TODO
 */
int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    struct inode in;

    if (inode_iget(fs, inumber, &in) < 0) {
        return -1;
    }

    int data_block = inode_indexlookup(fs, &in, blockNum);
    if (data_block == 0) {
        // Bloque no asignado
        return 0;
    }
    if (data_block < 0) {
        return -1;
    }

    int nbytes = diskimg_readsector(fs->dfd, data_block, buf);
    if (nbytes < 0) {
        return -1;
    }

    int filesize = (in.i_size0 << 16) | in.i_size1;
    int max_valid = filesize - blockNum * DISKIMG_SECTOR_SIZE;

    if (max_valid >= DISKIMG_SECTOR_SIZE) {
        return DISKIMG_SECTOR_SIZE;
    } else if (max_valid > 0) {
        return max_valid;
    } else {
        return 0;
    }
}


