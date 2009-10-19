//
//      FILE: Debug.C
//    AUTHOR: tab, bmc
//     DESCR: Debug log file utility functions
//

static char rcsid[] = "$Id: Debug.C 10 2002-07-09 12:24:59Z ejakowatz $";

#include "Debug.H"

#include <stdio.h>
#include <string.h>
#include <assert.h>

char dbg_file_list[DBG_MAX_FILES][DBG_MAX_FNAMELEN];
int dbg_num_files = 0;

#pragma init (dbg_init)

void dbg_init() {
   FILE *f;
   char *filename;
   if (!(filename = getenv (DBG_ENV_VAR)))
     filename = DBG_DEFAULT_FILE;

   if ((f = fopen(filename, "r")) == NULL) {
      return;
   }

   while ((!feof(f)) && (dbg_num_files < DBG_MAX_FILES)) {
      char buf[DBG_MAX_FNAMELEN];

      fgets(buf, DBG_MAX_FNAMELEN, f);
      if (buf) {

         // remove terminating newline
         if (buf[strlen(buf)-1] == '\n') {
            buf[strlen(buf)-1] = NULL;
         }

         if (buf[0] == 0)
            continue;

         strcpy(dbg_file_list[dbg_num_files], buf);
         dbg_num_files++;
      }
   }
   fclose(f);
}

int dbg_file_active(char *filename) {
   for (int i = 0; i<= dbg_num_files; i++) {
      if (strcasecmp(filename, dbg_file_list[i]) == 0) {
         return 1;
      }
   }
   return 0;
}
