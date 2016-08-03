#include "rinex2ubx.h"


/*--------------------------------------------------------------------------*/
//                          Date/Hour to week/iToW                          //
/*--------------------------------------------------------------------------*/
void toiToW(int year, int month, int day, int hour, int minute, float sec, int *week, int *iToW){
  int seconds;

  struct tm origin;
  origin.tm_sec = 0;
  origin.tm_min = 0;
  origin.tm_hour = 0;
  origin.tm_mday = 6;
  origin.tm_mon = 0;
  origin.tm_year = 1980 - 1900;
  origin.tm_zone = "UTC";
  origin.tm_isdst = 0;

  // La date qu'on veut calculer
  struct tm now;
  now.tm_sec = sec;
  now.tm_min = minute;
  now.tm_hour = hour;
  now.tm_mday = day;
  now.tm_mon = month-1;
  now.tm_year = year - 1900;
  now.tm_zone = "UTC";
  now.tm_isdst = 0;

  // calculer la différence entre les deux dates
  seconds = (time_t)mktime(&now) - (time_t)mktime(&origin);
  printf("Les secondes GPS: %d", seconds);
  // calculer la semaine gps
  *week = seconds / SECONDS_IN_WEEK;
  *iToW = (seconds % SECONDS_IN_WEEK)*1000;
}

/*--------------------------------------------------------------------------*/
//                             rinex2ubx function                           //
/*--------------------------------------------------------------------------*/

void rinex2ubx(FILE *rinex_file, FILE *ubx_file){
  size_t len = 0;
  char *line, tmp_string[3];
  ssize_t read;
  int week, iToW, year, month, day, hour, minute;
  float sec;
  
  // read header
  while((read = getline(&line, &len, rinex_file)) != -1){
    //printf("I read a line: %s\n", line);
    if(strstr(line, "END OF HEADER")){
      break;
    }
  }
  // read data
  while((read = getline(&line, &len, rinex_file)) != -1){
    if(strstr(line,"G")){
      printf("%s",line);
      toiToW(2016, 1, 1, 0, 0, 0.0, &week, &iToW);
      printf("week:%d, iToW:%d\n",week, iToW);
      // split line
      strncpy(tmp_string,line+1,2);
      tmp_string[2] = '\0';
      printf("EXTRACTED:%d\n",atoi(tmp_string));
    }
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
