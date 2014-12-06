#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include <errno.h>
#include <string.h>

#include <wiringPi.h>
#include <softPwm.h>

#define RANGE		100
#define	NUM_LEDS	  3

float convConst = 100 / 255.0;
int red = 0;
int blue = 1;
int green = 2;

int colorToRange(int c) {
  return c * convConst;
}

void setColor(int r, int g, int b) {
  int sr,sg,sb;
  printf("set color [%d,%d,%d]\n", r, g, b);
  sr = colorToRange(r);
  sg = colorToRange(g);
  sb = colorToRange(b);
  softPwmWrite(red, sr);
  softPwmWrite(green, sg);
  softPwmWrite(blue, sb);
}

void off() {
  softPwmWrite(red, 0);
  softPwmWrite(green, 0);
  softPwmWrite(blue, 0);
}

void setColorHex(int hex) {
  int r = hex>>16;
  int g = (hex & 0x00ff00)>>8;
  int b = hex & 0x0000ff;

  setColor(r,g,b);
}

int rgbToHex(int r, int g, int b) {
  return (r<<16)+(g<<8)+b;
}

int findColorInBuffer(char buffer[]) {
    char *c = strstr(buffer, "color\":[");
    if (c==0) {
      printf("no color info found, returning\n");
      return -1;
    }

    c += 8;
    char res[15];
    strncpy(res, c, 15);
    
    char alter[15];
    int is = 0;
    while (res[is] != '\0') {
      if (res[is] == ']') {
        break;
      }
      alter[is] = res[is];
      is++;
    }
    alter[is] = '\0';
    
    is = 0;
    int cp = 0;
    char red[4];
    char blue[4];
    char green[4];
    int state = 0;
    while (alter[is] != '\0') {
      if (alter[is] == ',') {
        state++;
        cp = 0;
        is++;
        continue;
      }
      if (state == 0) {
        red[cp] = alter[is];
        red[cp+1] = '\0';
      } else if (state == 1) {
        green[cp] = alter[is];
        green[cp+1] = '\0';
      } else {
        blue[cp] = alter[is];
        blue[cp+1] = '\0';
      }
      cp++;
      is++;
    }

   int r,g,b;
   r = atoi(red);
   g = atoi(green);
   b = atoi(blue);
   printf("red %d, green %d, blue %d\n", r, g, b);
   return rgbToHex(r,g,b);
}


//int main( int argc, char *argv[] )
int main()
{

  int i, j ;
  wiringPiSetup ()  ;

  for (i = 0 ; i < NUM_LEDS ; ++i)
  {
    softPwmCreate (red, 0, RANGE) ;
    softPwmCreate (blue, 0, RANGE) ;
    softPwmCreate (green, 0, RANGE) ;
  }


off();
setColor(255,0,0);
delay(1000);
setColor(0,255,0);
delay(1000);
setColor(0,0,255);
delay(1000);

    int sockfd, newsockfd, portno, clilen;
    char buffer[256];

    //int c = findColorInBuffer("{\"command\":\"color\",\"priority\":50,\"color\":[21,156,152],\"duration\":14400000}");
    //printf("%d\n", c);

    struct sockaddr_in serv_addr, cli_addr;
    int  n;

    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
        perror("ERROR opening socket");
        //exit(1);
    }
    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 19555;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
 
    /* Now bind the host address using bind() call.*/
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
                          sizeof(serv_addr)) < 0)
    {
         perror("ERROR on binding");
         exit(1);
    }



    /* Now start listening for the clients, here process will
    * go in sleep mode and will wait for the incoming connection
    */
    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    /* Accept actual connection from the client */
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, 
                                &clilen);
    if (newsockfd < 0) 
    {
        perror("ERROR on accept");
        //exit(1);
    }
    /* If connection is established then start communicating */
    while (1) {
      bzero(buffer,256);
      printf("wait for the shit\n");
      n = read( newsockfd,buffer,255 );
      if (n < 0) {
        perror("ERROR reading from socket");
        //exit(1);
      }
      printf("Here is the message: %s\n",buffer);

      int c = findColorInBuffer(buffer);

      /* Write a response to the client */
      if (c == -1) {
        printf("server connects, send info\n");
        n = write(newsockfd, "{'info': {'hostname': 'zeus'}, 'priorities': [], 'transform': [], 'success': 1, 'effects': []}\n",95);
      } else {
        setColorHex(c);
        n = write(newsockfd, "{'success': 1}\n", 15);
      }

      if (n < 0) {
        perror("ERROR writing to socket");
        //exit(1);
      }
    }
    return 0; 
}
