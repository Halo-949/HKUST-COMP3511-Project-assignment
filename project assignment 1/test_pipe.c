#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

int fd[2]; // file descriptors for a pipe
const int MAX_CHAR = 1000000; // any large integer is fine
void *reader() {
    char ch;
    int result, i;
    for (i = 0; i< MAX_CHAR; i++) {
        result = read(fd[0],&ch,1);
        printf("Reader: %c, count: %d\n", ch, i+1);
        fflush(stdout);
    }
    return NULL;
}
void *writer() {
    char ch = 'A';
    int result, i;
    for (i=0; i< MAX_CHAR; i++) {
        result = write(fd[1],&ch,1);
        printf("Writer: %c, count: %d\n", ch, i+1);
        fflush(stdout);
        if ( ch == 'Z')
            ch = 'A';
        else
            ch++;
    }
    return NULL;
}
int main() 
{
    pthread_t tid1, tid2;
    pipe(fd);
    pthread_create(&tid1,0,writer,NULL);
    sleep(10); // Sleep for 10 seconds. 
    pthread_create(&tid2,0,reader,NULL);
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    return 0;
}
