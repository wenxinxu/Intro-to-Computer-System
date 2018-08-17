#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>



#define DIRSIZE sizeof(struct direntv6)
/**
 * Looks up the specified name (name) in the specified directory (dirinumber).
 * If found, return the directory entry in space addressed by dirEnt.  Returns 0
 * on success and something negative on failure.
 */

int directory_findname(struct unixfilesystem *fs, const char *name,
		       int dirinumber, struct direntv6 *dirEnt) {
    struct inode node;
    // struct an inode to get inode info
    if(inode_iget(fs,dirinumber,&node) == -1) return -1;

    // if this is not a directory
    if((node.i_mode & IFMT) != IFDIR) return -1;
    // constrain on char in name less than 14
    if(strlen(name) > 14) return -1;

    int totalSize = inode_getsize(&node);

    int totalblk = (totalSize - 1)/DISKIMG_SECTOR_SIZE + 1;

    for(int i = 0; i < totalblk; i++) { //loop through all block

        int numOfDir = DISKIMG_SECTOR_SIZE/DIRSIZE;//number of DirEnt in a sector

        struct direntv6 dir[numOfDir];

        int validBytes = file_getblock(fs,dirinumber,i,&dir);
        //if bytes are not valid
        if (validBytes  == - 1) return -1;

        int validDir = validBytes/DIRSIZE;

        for(int i = 0; i < validDir;i++) {
            //found same name as required
            if(!strcmp(dir[i].d_name,name)) {
                strncpy((*dirEnt).d_name,name,strlen(name)*sizeof(char));
                (*dirEnt).d_inumber = dir[i].d_inumber;
                return  0;
            }
        }
    }
    return -1;

}
