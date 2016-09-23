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



//***************************************************************************//
//            Fonction pour le calcul du checksum                            //
//***************************************************************************//
void getCompleteChecksum(ubx_message *ptr_ubx){
  // probleme dans cette fonction: les checksum sont variables
  // Algorithme de Fletcher: voir page 86 de u-blox6 receiver description protocol
  unsigned short int length = (ptr_ubx->message_length[1]<<8)|ptr_ubx->message_length[0];
  ptr_ubx->checksum_a = 0x00;
  ptr_ubx->checksum_b = 0x00;
  ptr_ubx->checksum_a = ptr_ubx->checksum_a + ptr_ubx->message_class;
  ptr_ubx->checksum_b = ptr_ubx->checksum_b + ptr_ubx->checksum_a;
  ptr_ubx->checksum_a = ptr_ubx->checksum_a + ptr_ubx->message_id;
  ptr_ubx->checksum_b = ptr_ubx->checksum_b + ptr_ubx->checksum_a;
  ptr_ubx->checksum_a = ptr_ubx->checksum_a + ptr_ubx->message_length[0];
  ptr_ubx->checksum_b = ptr_ubx->checksum_b + ptr_ubx->checksum_a;
  ptr_ubx->checksum_a = ptr_ubx->checksum_a + ptr_ubx->message_length[1];
  ptr_ubx->checksum_b = ptr_ubx->checksum_b + ptr_ubx->checksum_a;
  for(int i=0; i<length; i++){
    ptr_ubx->checksum_a = ptr_ubx->checksum_a + ptr_ubx->payload[i];
    ptr_ubx->checksum_b = ptr_ubx->checksum_b + ptr_ubx->checksum_a;
  }
  //checksum(ptr_ubx.id[0], ptr_ubx->checksum_a, ptr_ubx->checksum_b);
}


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
void getObsPos(int *number_of_obs_lines, int *posL1, int *posC1, int *posD1, int *posS1,char *line){
  printf("THELINE: %s\n",line);
  char tmp_string[7];
  tmp_string[7] = 0;
  int number_of_lines;

  strncpy(tmp_string, line, 6);
  printf("Nombre d'observables: %d\n", atoi(tmp_string));
  // Determine number of # / TYPE OF OBS lines
  number_of_lines = (int) ceil((float)atoi(tmp_string)/9);
  printf("Nombre de lignes: %d\n", number_of_lines);
  printf("OBSERVABLES : \n");
  // Nombre de lignes d observations par satellite
  *number_of_obs_lines = (int) ceil((float)atoi(tmp_string)/5);

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
  ubx_message ubx_msg;
  size_t len = 0;
  char *line, tmp_string[38], tmp_string_2[38], tmp_short_string[4];
  ssize_t read;
  int week, iToW, year, month, day, hour, minute, numSV;
  int sv, number_of_obs_lines;
  int posL1, posC1, posD1, posS1;
  float sec;
  double double_null = 0.0;
  union L1{
    double ascii;
    unsigned char bin[8];
  }l1;

  union C1{
    double ascii;
    unsigned char bin[8];
  }c1;

  union D1{
    double ascii;
    unsigned char bin[8];
  }d1;

  union S1{
    int ascii;
    unsigned char bin[4];
  }s1;


  ubx_msg.header[0] = 0xb5;
  ubx_msg.header[1] = 0x62;


  // read header
  while((read = getline(&line, &len, rinex_file)) != -1){
    //printf("I read a line: %s\n", line);
    if(strstr(line, "# / TYPES OF OBSERV")){
      getObsPos(&number_of_obs_lines, &posL1, &posC1, &posD1, &posS1, line);
      printf("Les positions de chaque observable:\nL1:%d\nC1:%d\nD1:%d\nS1:%d\n",posL1, posC1, posD1, posS1);
    }
    else if(strstr(line, "END OF HEADER")){
      break;
    }
    else if(strstr(line, "APPROX POSITION XYZ")){
      printf("%s\n",line);
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

      year += 2000;

      toiToW(year, month, day, hour, minute, sec, &week, &iToW);
      printf("WEEK:%d SEC:%d\n", week, iToW);

      // Write UTCTIME-MESSAGE
      ubx_msg.message_class = 0x01;
      ubx_msg.message_id = 0x21;

      ubx_msg.message_length[0] = 0x14;
      ubx_msg.message_length[1] = 0x00;

      // copy du iToW dans la mémoire
      ubx_msg.payload[0] = iToW >> 0;
      ubx_msg.payload[1] = iToW >> 8;
      ubx_msg.payload[2] = iToW >> 16;
      ubx_msg.payload[3] = iToW >> 24;

      // copy de Time Accuracy Estimate (dans ce cas on met 0)
      ubx_msg.payload[4] = 0x00;
      ubx_msg.payload[5] = 0x00;
      ubx_msg.payload[6] = 0x00;
      ubx_msg.payload[7] = 0x00;

      // copy de nanosec of sec
      ubx_msg.payload[8] = 0x00;
      ubx_msg.payload[9] = 0x00;
      ubx_msg.payload[10] = 0x00;
      ubx_msg.payload[11] = 0x00;

      // copy of year
      ubx_msg.payload[12] = year >> 0;
      ubx_msg.payload[13] = year >> 8;

      // copy of month
      ubx_msg.payload[14] = month >> 0;

      // copy of day
      ubx_msg.payload[15] = day >> 0;

      // copy of hour
      ubx_msg.payload[16] = hour >> 0;

      // copy of hour
      ubx_msg.payload[17] = minute >> 0;

      // copy of sec of minute
      ubx_msg.payload[18] = (int) sec >> 0;

      // set validity flag
      ubx_msg.payload[19] = 7 >> 0;

      getCompleteChecksum(&ubx_msg);

      // Write UTC message in the file
      fwrite( &ubx_msg, sizeof(unsigned char), 6, ubx_file );
      fwrite( ubx_msg.payload, sizeof(unsigned char), 20, ubx_file );
      fwrite( &(ubx_msg.checksum_a), sizeof(unsigned char), 1, ubx_file );
      fwrite( &(ubx_msg.checksum_b), sizeof(unsigned char), 1, ubx_file );

      // Sat number
      strncpy(tmp_string,line+30,2);
      tmp_string[2] = '\0';
      numSV  = atoi(tmp_string);
      printf("numSV=%d\n",numSV);

      ubx_msg.message_class = 0x02;
      ubx_msg.message_id = 0x10;

      ubx_msg.message_length[0] = (8+24*numSV) >> 0;
      ubx_msg.message_length[1] = (8+24*numSV) >> 8;

      ubx_msg.payload[0] = iToW >> 0;
      ubx_msg.payload[1] = iToW >> 8;
      ubx_msg.payload[2] = iToW >> 16;
      ubx_msg.payload[3] = iToW >> 24;

      ubx_msg.payload[4] = week >> 0;
      ubx_msg.payload[5] = week >> 8;

      ubx_msg.payload[6] = numSV >> 0;

      ubx_msg.payload[7] = 0 >> 0;
      //fwrite( &ubx_msg, sizeof(unsigned char), 6, ubx_file);
      strncpy(tmp_string, line+32, 36);
      tmp_string[36] = 0;
      logPrint(4,"SATS: %s\n",tmp_string);

      // Repeat block for every SV
      if(numSV < 12){
        // loop for satellites from 0 to satellites number
        strcpy(tmp_string_2, tmp_string); // tmp_string contient les PRN des satellites
        for( int i=0; i<numSV; i++){
          strncpy(tmp_short_string, tmp_string_2+i*3,3);
          tmp_short_string[3] = 0;
          sv = atoi(strtok(tmp_short_string,"G"));
          printf("DEBUG sv: %s\n", tmp_short_string);
          printf("-----------UN STALLITE %d-------------\n", sv);
            for( int j = 0 ; j<number_of_obs_lines; j++ ){
              read = getline(&line, &len, rinex_file);
              printf("%s", line);
              for ( int k = 0 ; k<5; k++ ){
                if ( ( posL1 -1 ) == j*5+k ){

                  strncpy(tmp_string, line+k*16, 14);
                  tmp_string [14] = 0;
                  l1.ascii = (double) atof(tmp_string);
                  printf("L1 PARSED: %.14g\n", l1.ascii);
                  memcpy((ubx_msg.payload)+(8+24*numSV), &(l1.bin), 8);
                  // lli
                  strncpy(tmp_string, line+k*17, 1);
                  tmp_string [1] =0;
                  ubx_msg.payload [31+24*numSV] = atoi(tmp_string) >> 0;

                } else if ( ( posC1 -1 ) == j*5+k ) {

                  strncpy(tmp_string, line+k*16, 14);
                  tmp_string [14] = 0;
                  c1.ascii = (double) atof(tmp_string);
                  printf("C1 PARSED: %.14g\n", c1.ascii);
                  memcpy((ubx_msg.payload)+(16+24*numSV), &(c1.bin), 8);

                } else if ( ( posD1 -1 ) == j*5+k ) {

                  strncpy(tmp_string, line+k*16, 14);
                  tmp_string [14] = 0;
                  d1.ascii = (double) atof(tmp_string);
                  printf("D1 PARSED: %.14g\n", d1.ascii);
                  memcpy((ubx_msg.payload)+(24+24*numSV), &(d1.bin), 4);

                } else if ( ( posS1 -1 ) == j*5+k ) {

                  strncpy(tmp_string, line+k*16, 14);
                  tmp_string [14] = 0;
                  s1.ascii = (int) atof(tmp_string);
                  printf("S1 PARSED: %d\n", s1.ascii);
                  ubx_msg.payload [30 + 24*numSV] = s1.bin[0] ;

                }
                // if one of observations was not found in the RINEX file
                if ( posD1 == 0 ) {
                  memcpy(ubx_msg.payload + ( 24 + 24 * numSV ), &double_null, 8) ;
                } if ( posS1 == 0 ) {
                  memcpy(ubx_msg.payload + ( 30 + 24 * numSV ), &double_null, 8) ;
                } if ( posL1 == 0 ) {
                  memcpy(ubx_msg.payload + ( 8 + 24 * numSV ), &double_null, 8) ;
                } if ( posC1 == 0 ) {
                  memcpy(ubx_msg.payload + ( 16 + 24 * numSV ), &double_null, 8) ;
                }
                //write SV prn
                ubx_msg.payload[28+24*numSV] = sv >> 0;

                // mesQI
                ubx_msg.payload[29+24*numSV] = 6 >> 0;

              }

            }
        }
        fwrite( &ubx_msg, sizeof(unsigned char), 6+2+8+numSV*24, ubx_file);

        // loop for satellites from 12
        //fwrite( &ubx_msg, sizeof(unsigned char), 6+8+numSV*24, ubx_file);
      } else {
        for(int i=0; i<12; i++){
          strncpy(tmp_short_string, tmp_string+i*3, 3);
          tmp_short_string[3] = 0;
          printf("DEBUG :%s\n",tmp_short_string);
        }

        // read next line
        read = getline(&line, &len, rinex_file);

        printf("SATS2:%s\n", line);

        for(int i=12; i<numSV; i++){
          strncpy(tmp_short_string, line+32+(i-12)*3, 3);
          tmp_short_string[3] = 0;
          printf("DEBUG: %s\n", tmp_short_string);

        }
        // After reading all satellites

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
  ubx_file = fopen(argv[2], "ab");

  if(rinex_file != 0){
    rinex2ubx(rinex_file, ubx_file);
    exit(EXIT_SUCCESS);
  } else {
    printf("A problem occured when trying to Open Rinex file\n");
    exit(EXIT_FAILURE);
  }
}
