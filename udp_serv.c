#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>

extern int errno;
int main(int argc , char **argv)
{
  int sock_fd = -1;
  struct sockaddr_in local_addr;
  struct sockaddr_in cli_addr;
  socklen_t len;
  
  int ret = -1;  
  char buff[2048] = {0};
  //create
  sock_fd = socket(AF_INET , SOCK_DGRAM , 0);
  if(sock_fd < 0)
  {
    printf("create socket failed!\n");
    return -1;
  }

  //bind
  memset(&local_addr , 0 , sizeof(local_addr));
  local_addr.sin_family = AF_INET;
  local_addr.sin_port = htons(7777);
  local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  ret = bind(sock_fd , (struct sockaddr *)&local_addr , sizeof(local_addr));
  if(ret < 0)
  {
    printf("bind failed! err:%s\n" , strerror(errno));
    return -1;
  }

  //read and print
  while(1)
  {
    memset(buff , 0 , sizeof(buff));
    ret = recvfrom(sock_fd , buff , sizeof(buff) , 0 , (struct sockaddr *)&cli_addr , &len);
    if(ret < 0)
    {
      printf("recv error! err:%s\n" , strerror(errno));
    }
    /*
    else if(ret == 0)
    {
      printf("peer closed\n");
      continue;
    }
    */
    
    //print
    printf("buff[%d]>>>%s" , ret , buff);
    fflush(stdout);
  }

  close(sock_fd);
  return 0;
}
