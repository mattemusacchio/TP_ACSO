#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "inode.h"
#include "diskimg.h"


/**
 * TODO
 */
int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
    if (inumber < 1 || inp == NULL) {
    return -1;  
    }
    int inodes_per_block = DISKIMG_SECTOR_SIZE / sizeof(struct inode);
    int block_offset = (inumber - 1) / inodes_per_block;
    int inode_offset = (inumber - 1) % inodes_per_block;

    int sector = INODE_START_SECTOR + block_offset;
    char buffer[DISKIMG_SECTOR_SIZE];

    if (diskimg_readsector(fs->dfd, sector, buffer) < 0){
        perror("inode_iget: error leyendo sector");
        return -1;
    }

    struct inode *inodes = (struct inode *)buffer;
    *inp = inodes[inode_offset];

    return 0;

}


/**
 * TODO
 */
int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp,int blockNum) {  
    if ((inp->i_mode & ILARG) == 0) {
    // No es large: acceso directo
    if (blockNum < 0 || blockNum >= 8) return -1;
    return inp->i_addr[blockNum];
    }
    // Es large
    if (blockNum < 0) return -1;

    char block[DISKIMG_SECTOR_SIZE];

    if (blockNum < 7 * 256) {
        // Indirectos simples
        int indirect_index = blockNum / 256;
        int offset = blockNum % 256;
        int indirect_block = inp->i_addr[indirect_index];

        if (diskimg_readsector(fs->dfd, indirect_block,block ) < 0) {
            return -1;
        }

        unsigned short *entries = (unsigned short *)block;
        return entries[offset];
    }

    // Doble indirecto (i_addr[7])
    int remaining = blockNum - 7 * 256;
    int indirect1 = remaining / 256;
    int indirect2 = remaining % 256;

    int dind_block = inp->i_addr[7];
    if (diskimg_readsector(fs->dfd, dind_block,block ) < 0) {
        return -1;
    }

    unsigned short *level1 = (unsigned short *)block;
    int indir_block = level1[indirect1];

    if (diskimg_readsector(fs->dfd, indir_block,block) < 0) {
        return -1;
    }

    unsigned short *level2 = (unsigned short *)block;
    return level2[indirect2];
}

int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}