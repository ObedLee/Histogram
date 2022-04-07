/* 크기가 <size> byte인 <number> 개의 데이터 파일을 생성한다. 첫 인자가 'c'이면 첫 파일의 값은 1, 두 번째 파일의 값은 2, 이런 식으로 채워지고, 'r'이면 랜덤하게 채워진다. 
   compile: gcc -g -Wall -o random random.c
*/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

unsigned char *buffer; 

int main(int argc, char *argv[])
{
  if (argc != 4){
    fprintf(stderr, "Usage: %s <c/r> <size> <number>\n", argv[0]);
    exit(1);
  }
  char type = argv[1][0];
  long int size = atoi(argv[2]);
  int number = atoi(argv[3]);
  buffer = (unsigned char *)malloc(sizeof(char) * size);
  assert(buffer);
  srandom(0);

  for (int i = 1; i <= number; i++){
    char fn[128];
    
    sprintf(fn, "data%d.bin", i);
    int fd = creat(fn, 0666);
    assert(fd);
    if (type == 'c') {
      for (int j = 0; j < size; j++)
	buffer[j] = (unsigned char)i;
    } else { /* type == 'r' */
      for (int j = 0; j < size; j++)
	buffer[j] = (unsigned char)random();
    }
    write(fd, buffer, size);
    close(fd);
  }
  return(0);
}
