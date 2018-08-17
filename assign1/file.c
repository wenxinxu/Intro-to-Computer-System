#include <stdio.h>
#include <assert.h>

#include "file.h"
#include "inode.h"
#include "diskimg.h"

// remove the placeholder implementation and replace with your own
int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    struct inode node;
    if(inode_iget(fs,inumber,&node) == - 1) return -1;

    //validate the incomming file's block is valid
    int block = inode_indexlookup(fs,&node,blockNum);
    if(block == -1) return -1;
    if(diskimg_readsector(fs->dfd,block,buf) == -1) return -1;

    size_t size = inode_getsize(&node);
    int usedBlock = size/DISKIMG_SECTOR_SIZE;

    if(blockNum == usedBlock) {
        return size % DISKIMG_SECTOR_SIZE;
    } else {
        return DISKIMG_SECTOR_SIZE;
    }



}
