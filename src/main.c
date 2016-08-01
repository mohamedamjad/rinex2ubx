#include "rinex2ubx.h"


/*--------------------------------------------------------------------------*/
//                             rinex2ubx function                           //
/*--------------------------------------------------------------------------*/

void rinex2ubx(FILE *rinex_file, FILE *ubx_file){
  size_t len = 0;
  char *line;
  ssize_t read;

  while((read = getline(&line, &len, rinex_file)) != -1){
    //printf("I read a line: %s\n", line);
    if(){

    }
    if(strstr(line,"#")) printf("I read a line with #: %s\n", line);
  }
  fclose(rinex_file);
  fclose(ubx_file);
}
/*--------------------------------------------------------------------------*/
//                             Main function                                //
/*--------------------------------------------------------------------------*/

int main(int argc, char* argv[]){

  FILE *rinex_file, *ubx_file;
  rinex_file = fopen(argv[1], "r");
  ubx_file = fopen(argv[2], "a");

  if(rinex_file != 0){
    rinex2ubx(rinex_file, ubx_file);
    exit(EXIT_SUCCESS);
  } else {
    printf("A problem occured when trying to Open Rinex file\n");
    exit(EXIT_FAILURE);
  }
}
