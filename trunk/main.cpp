#include "telnet.h"
#include <stdio.h>
#include <string.h>
#include <direct.h>

#ifdef WIN32
#pragma warning(disable:4996)
#include <conio.h>
#endif


bool getInputString(char *buf,int len)
{
  bool ret = true;

  int index = 0;
  bool exit = false;
  while ( !exit )
  {
    unsigned int client;
    const char *input = gTelnet->receiveMessage(client);
    if ( input )
    {
      printf("[%d] %s\r\n", client, input );
    }
    if ( kbhit() )
    {
      char c = (char)getch();
      if ( c == 10 || c == 13 )
      {
        buf[index] = 0;
        exit = true;
      }
      else if ( c == 27 )
      {
        buf[index] = 0;
        exit = true;
        ret = false;
      }
      else if ( c == 8 )
      {
        if ( index )
        {
          index--;
          printf("%c",c);
          printf(" ",c);
          printf("%c",c);
        }
      }
      {
        if ( c >= 32 && c <= 127 )
        {
          buf[index] = c;
          index++;
          printf("%c",c);
          if ( index == (len-1) )
            break;
        }
      }
    }
  }

  buf[index] = 0;

  printf("\n");

  return ret;
}

void main(int /* argc */,const char **argv)
{
  char dirname[512];
  strncpy(dirname,argv[0],512);
  int len = (int)strlen(dirname);
  char *scan = &dirname[len-1];
  while ( len )
  {
    if ( *scan == '\\' )
    {
      *scan = 0;
      break;
    }
    scan--;
    len--;
  }

  chdir( dirname );

  gTelnet = TELNET::createTelnet();


  if ( gTelnet->haveConnection() )
  {
    if ( gTelnet->isServer() )
    {
      printf("Created a Telnet Server\r\n");
    }
    else
    {
      printf("Created a Telnet Client.\r\n");
    }

    printf("Type a message and hit 'enter' to send it.\r\n");
    printf("Type 'bye' and hit enter to quit (or the escape key)\r\n");

    while ( 1 )
    {
      char buff[512];
      if ( getInputString(buff,512) )
      {
        if ( stricmp(buff,"bye") == 0 )
        {
          break;
        }
        gTelnet->sendMessage(0,"%s\r\n", buff );
      }
      else
        break;
    }
  }
  else
  {
    printf("Unable to establish a telnet connection!\r\n");
  }

  TELNET::releaseTelnet(gTelnet);

}
