
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>

#ifdef WIN32
#pragma warning(disable:4996)
#include <conio.h>
#endif

#include "telnet.h"

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
            //printf("[%d] %s\r\n", client, input );
	        printf("%s", input );
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

    buf[index] = 0;
    printf("\n");
    return ret;
}

int main(int argc, const char **argv)
{
    if(argc == 1)
    {
        gTelnet = TELNET::createTelnetClient("127.0.0.1", 23);
    }
    else if(argc == 4)
    {
        // host, port, type(server or client)
        const char* host = argv[1];
        printf("Host is: %s\r\n", host);
        const char* charPort = argv[2];
        int port = atoi(charPort);
        printf("port is: %s\r\n", charPort);
        const char* type = argv[3];
        printf("Type is: %s\r\n", type);
        
        if(stricmp(type, "server") == 0)
        {
            gTelnet = TELNET::createTelnetServer(host, port);
        }
        else if(stricmp(type, "client") == 0)
        {
            gTelnet = TELNET::createTelnetClient(host, port);
        }
        else
        {
            printf("Error>>>Parameter Type is not right, you should according the rule:\r\n");
            printf("Host port [server/client]\r\n");
            printf("Please hit any key to end the program and try again.");
            getchar();
            return -1;
        }
    }
    else
    {
        printf("Error>>>Parameters is not right, you should according the rule:\r\n");
        printf("Host port [server/client]\r\n");
        printf("Please hit any key to end the program and try again.");
        getchar();
        return -1;
    }

    if(!gTelnet)
    {
        printf("Error>>>Created a Telnet Server or Client failed.\r\nPlease make sure that the iP and port is right!\r\n");
        printf("Please hit any key to end the program.");
        getchar();
        return -2;
    }

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
            {
                break;
            }  
        }
    }
    else
    {
        printf("Unable to establish a telnet connection!\r\n");
    }

    TELNET::releaseTelnet(gTelnet);
    //return 0;
}
