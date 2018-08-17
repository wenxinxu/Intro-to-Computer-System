
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * Returns the inode number associated with the specified pathname.  This need only
 * handle absolute paths.  Returns a negative number (-1 is fine) if an error is
 * encountered.
 */

int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {

    int dirNum = ROOT_INUMBER;
    char path[strlen(pathname) + 1];
    strncpy(path,pathname,strlen(pathname));
    path[strlen(pathname)] = '\0';
  if(!strcmp(pathname,"/")) {

      return dirNum;
  }
  else {

      char * sub = strtok(path,"/");
      while(sub != NULL) {
          struct direntv6 Dir;
          if(directory_findname(fs,sub,dirNum,&Dir) != -1) {

              dirNum  = Dir.d_inumber;
          }
          else {
              return -1;
          }
          sub = strtok(NULL,"/");
      }
      return dirNum;

  }

}
