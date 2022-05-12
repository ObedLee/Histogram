#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_USEC 1000000

typedef unsigned char BYTE;
typedef struct timeval timeval;

int get_filesize(const char *filename);
timeval get_subtime(timeval *time1, timeval *time2);

const char hisFileName[] = "histogram.bin";

int main(int argc, char *argv[])
{
    // arg 값이 올바른지 확인
    if (argc != 3 && argc != 4)
    {
        fprintf(stderr, "Usage: %s <start number> <end number> <child number>\n", argv[0]);
        fprintf(stderr, "Correct example: %s 1 128 10\n", argv[0]);
        exit(1);
    }
    else if (!(isdigit(argv[1][0]) && isdigit(argv[2][0])))
    {
        fprintf(stderr, "인수가 올바르지 않습니다.(is not a positive number)\n");
        exit(1);
    }

    int sNum, eNum;              // 파일의 시작번호, 끝번호
    int fCount, fshare, fremain; // 파일개수, 파일개수 몫, 파일개수 나머지
    int childNum = 1;            // 자식 프로세스 개수

    if (argc == 4 && isdigit(argv[3][0]))
        childNum = atoi(argv[3]);
    sNum = atoi(argv[1]);
    eNum = atoi(argv[2]);
    fCount = eNum - sNum + 1;
    fshare = fCount / childNum;
    fremain = fCount % childNum;

    int state;     // wait 용 상태 변수
    pid_t pid = 0; // 프로세스 id
    int tidx = 0;  // 자식 프로세스 인덱스
    int fnum = 0;  // 자식 프로세스의 파일 개수
    int fidx[2];   // 자식 프로세스의 파일 인덱스(시작, 종료)

    int hisSum[256] = {0};
    char filename[20];
    int filesize;
    int hisfd, fd;
    BYTE *dataPtr;

    timeval pSTime, pETime, pRTime; // 부모 프로세스의 시작시간, 종료시간, 수행시간
    timeval cSTime, cETime, cRTime; // 자식 프로세스의 시작시간, 종료시간, 수행시간

    gettimeofday(&pSTime, NULL); // 부모 시작시간 저장

    for (int i = 0; i < childNum; i++)
    {
        // 자식 프로세스 생성 및 인덱스 설정
        tidx = i;
        pid = fork();

        if (pid < 0)
        { // fork 실패
            printf("error: fork Fail! \n");
            return -1;
        }
        else if (pid == 0)
        { // 자식 코드

            fnum = fshare;
            if (fremain > tidx)
            {
                fnum++;
            }

            fidx[0] = sNum + tidx * fshare;
            if (tidx < fremain)
            {
                fidx[0] += tidx;
            }
            else
            {
                fidx[0] += fremain;
            }
            fidx[1] = fidx[0] + fnum - 1;

            gettimeofday(&cSTime, NULL); // 자식 시작시간 저장

            for (int j = fidx[0]; j <= fidx[1]; j++)
            {
                // histogram.bin 내용을 hisSum에 저장
                hisfd = open(hisFileName, O_RDWR);
                if (hisfd == -1)
                {
                    perror("파일 열기 실패\n");
                    exit(1);
                }
                lockf(hisfd, F_LOCK, sizeof(hisSum));
                read(hisfd, hisSum, sizeof(hisSum));
                lockf(hisfd, F_ULOCK, sizeof(hisSum));
                close(hisfd);

                // data000.bin 파일 열기
                sprintf(filename, "data%d.bin", j);
                fd = open(filename, O_RDONLY);
                if (fd == -1)
                {
                    perror("파일 열기 실패\n");
                    exit(1);
                }

                // 파일 크기만큼 동적할당
                filesize = get_filesize(filename);
                dataPtr = malloc(filesize);
                read(fd, dataPtr, filesize);
                close(fd);

                // hisSum 배열 elementwise 덧셈
                for (int k = 0; k < filesize; k++)
                {
                    hisSum[dataPtr[k]] += 1;
                }
                free(dataPtr);

                // hisSum 내용을 histogram.bin에 저장
                hisfd = open(hisFileName, O_RDWR);
                if (hisfd == -1)
                {
                    perror("파일 열기 실패\n");
                    exit(1);
                }
                lockf(hisfd, F_LOCK, sizeof(hisSum));
                write(hisfd, hisSum, sizeof(hisSum));
                lockf(hisfd, F_ULOCK, sizeof(hisSum));
                close(hisfd);
            }

            gettimeofday(&cETime, NULL); // 자식 종료시간 저장

            cRTime = get_subtime(&cSTime, &cETime);

            printf("%d번째 자식 프로세스 수행시간 : %lds %dus\n", tidx, cRTime.tv_sec, cRTime.tv_usec);

            exit(0);
        }
    }

    while (wait(&state) > 0)
        ;

    gettimeofday(&pETime, NULL); // 부모 종료시간 저장
    pRTime = get_subtime(&pSTime, &pETime);
    printf("부모 프로세스 수행시간 : %ldsec %dusec\n", pRTime.tv_sec, pRTime.tv_usec);

    return 0;
}

// 파일 사이즈를 반환
int get_filesize(const char *filename)
{
    int filesize = 0;

    FILE *fp = fopen(filename, "r");

    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);

    fclose(fp);

    return filesize;
}

// 시간의 차를 timeval 구조체로 반환
timeval get_subtime(timeval *time1, timeval *time2)
{
    timeval subTime = (timeval){0};

    subTime.tv_sec = time1->tv_sec - time2->tv_sec;
    subTime.tv_usec = time1->tv_usec - time2->tv_usec;

    if (subTime.tv_sec < 0 || (subTime.tv_sec == 0 && subTime.tv_usec < 0))
    {
        subTime.tv_sec = -subTime.tv_sec;
        subTime.tv_usec = -subTime.tv_usec;
    }

    while (subTime.tv_usec < 0)
    {
        subTime.tv_sec--;
        subTime.tv_usec += MAX_USEC;
    }

    return subTime;
}
