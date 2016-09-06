#include "rinex2ubx.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#define LOG_HEADER 1
#define LOG_SAT 2
#define LOG_OBS 4

int type_log = LOG_SAT;
/*-------------------------------------------------------------------------*/
//                            Log Debug function                            //
/*--------------------------------------------------------------------------*/
__attribute__ (( format (printf, 2, 3) ))
void logPrint(int type, const char *fmt, ...)
{
  va_list ap;

  va_start(ap,fmt);
  if (type & type_log) {
    vprintf(fmt,ap);
  }
  va_end(ap);

}

/*--------------------------------------------------------------------------*/
//                          Get L1/C1/D1/S1 positions                       //
/*--------------------------------------------------------------------------*/
void getObsPos(int *posL1, int *posC1, int *posD1, int *posS1,char *line){
  printf("THELINE: %s\n",line);
  char tmp_string[7];
  int number_of_lines;
  tmp_string[7] = 0;

  strncpy(tmp_string, line, 6);
  printf("Nombre d'observables: %d\n", atoi(tmp_string));
  // Determine number of # / TYPE OF OBS lines
  number_of_lines = (int) ceil((float)atoi(tmp_string)/9);
  printf("Nombre de lignes: %d\n", number_of_lines);
  printf("OBSERVABLES : \n");
  for ( int i=0; i<number_of_lines; i++){
    // Read a line after each loop cycle
    for(int j = i*10; j < (i*10)+9; j++){
      strncpy(tmp_string, line+6+j*6, 6);
      tmp_string[7] = 0;
      if(strstr(tmp_string, "L1")){
        *posL1 = j+1;
      }else if(strstr(tmp_string, "C1")){
        *posC1 = j+1;
      }else if(strstr(tmp_string, "D1")){
        *posD1 = j+1;
      }else if(strstr(tmp_string, "S1")){
        *posS1 = j+1;
      }
    }
  }
}

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
  // calculer la semaine gps
  *week = seconds / SECONDS_IN_WEEK;
  *iToW = (seconds % SECONDS_IN_WEEK)*1000;
}

/*--------------------------------------------------------------------------*/
//                             rinex2ubx function                           //
/*--------------------------------------------------------------------------*/

void rinex2ubx(FILE *rinex_file, FILE *ubx_file){
  size_t len = 0;
  char *line, tmp_string[38], tmp_short_string[4];
  ssize_t read;
  int week, iToW, year, month, day, hour, minute, numSV;
  int sv;
  int posL1, posC1, posD1, posS1;
  float sec;
  
  // read header
  while((read = getline(&line, &len, rinex_file)) != -1){
    //printf("I read a line: %s\n", line);
    if(strstr(line, "# / TYPES OF OBSERV")){
      getObsPos(&posL1, &posC1, &posD1, &posS1, line);
      printf("Les positions de chaque observable:\nL1:%d\nC1:%d\nD1:%d\nS1:%d\n",posL1, posC1, posD1, posS1);
    }
    if(strstr(line, "END OF HEADER")){
      break;
    }
  }

  // read data
  while((read = getline(&line, &len, rinex_file)) != -1){
    if(strstr(line,"G") && strncmp("       ",line,7)<0){
      printf("%s",line);
      //toiToW(2016, 1, 1, 0, 0, 0.0, &week, &iToW);
      //printf("week:%d, iToW:%d\n",week, iToW);

      // split line epoch header line
      // get date/hour infos
      strncpy(tmp_string,line+1,2);
      tmp_string[2] = '\0';
      year = atoi(tmp_string);

      strncpy(tmp_string,line+4,2);
      month = atoi(tmp_string);

      strncpy(tmp_string,line+7,2);
      day = atoi(tmp_string);

      strncpy(tmp_string,line+10,2);
      hour = atoi(tmp_string);

      strncpy(tmp_string,line+13,2);
      minute = atoi(tmp_string);

      strncpy(tmp_string,line+16,10);
      sec = atof(tmp_string);

      toiToW(2000+year, month, day, hour, minute, sec, &week, &iToW);
      printf("WEEK:%d SEC:%d\n", week, iToW);

      // Sat number
      strncpy(tmp_string,line+30,2);
      tmp_string[2] = '\0';
      numSV  = atoi(tmp_string);
      printf("numSV=%d\n",numSV);
      
      strncpy(tmp_string, line+32, 36);
      tmp_string[36] = 0;
      logPrint(1,"SATS: %s\n",tmp_string);

      // Repeat block for every SV
      if(numSV < 12){
        // loop for satellites from 0 to satellites number
        for( int i=0; i<numSV; i++){
          strncpy(tmp_short_string, tmp_string+i*3,3); // tmp_string contient les PRN des satellites
          printf("DEBUG ");
          
        }
        
        // loop for satellites from 12
      } else {
        for(int i=0; i<12; i++){
          
        }

        // read next line
        read = getline(&line, &len, rinex_file);
        printf("SATS2:%s\n", line);
        for(int i=12; i<numSV; i++){
          i++;
        }
      }
      
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
