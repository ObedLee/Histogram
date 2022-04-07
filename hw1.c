#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/time.h>

#define MAX_USEC 1000000

typedef unsigned char BYTE;
typedef struct timeval timeval;

timeval get_subtime(timeval *time1, timeval *time2);
timeval get_sumtime(timeval *time1, timeval *time2);
timeval get_avgtime(timeval *time, const int num);
timeval get_multitime(timeval *time1, timeval *time2);
timeval get_stddevtime(timeval time[], timeval *avg, const int count);
int get_filesize(const char *filename);
double get_sqrt(long int num);

int main(int argc, char *argv[])
{

    // arg 값이 올바른지 확인
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <time1 number> <time2 number>\n", argv[0]);
        fprintf(stderr, "Correct example: %s 1 128\n", argv[0]);
        exit(1);
    }
    else if (!(isdigit(argv[1][0]) && isdigit(argv[2][0])))
    {
        fprintf(stderr, "인수가 올바르지 않습니다.(is not a positive number)\n");
        exit(1);
    }

    int sNum, eNum, count; // 파일의 시작번호, 끝번호, 파일개수

    sNum = atoi(argv[1]);
    eNum = atoi(argv[2]);
    count = eNum - sNum + 1;

    char filename[20];
    int filesize; // 파일크기
    int hisSum[256];
    BYTE *hisPtr;
    FILE *hisfp, *fp;
    timeval sTime, eTime;                      // 프로세스의 시작시간, 종료시간
    timeval totalTime = (timeval){0}, avgTime; // 총 수행시간, 평균 수행시간
    timeval runTime[count];                    // 파일별 수행시간 배열
    timeval stdDevTime;                        // 수행시간 표준편차

    for (int i = sNum; i <= eNum; i++)
    {
        gettimeofday(&sTime, NULL); // 시작시간 저장

        // histogram.bin 내용을 hisSum에 저장
        hisfp = fopen("histogram.bin", "rb");
        if (hisfp == NULL)
        {
            fprintf(stderr, "파일 열기 실패\n");
            exit(1);
        }
        fread(hisSum, 1, sizeof(hisSum), hisfp);
        fclose(hisfp);

        // data000.bin 파일 열기
        sprintf(filename, "./data/data%d.bin", i);
        fp = fopen(filename, "rb");
        if (fp == NULL)
        {
            fprintf(stderr, "파일 열기 실패\n");
            exit(1);
        }

        // 파일 크기만큼 동적할당하여 데이터 저장
        filesize = get_filesize(filename);
        hisPtr = malloc(filesize);
        fread(hisPtr, 1, filesize, fp);
        fclose(fp);

        // hisSum 배열 elementwise 덧셈
        for (int j = 0; j < filesize; j++)
        {
            hisSum[hisPtr[j]] += 1;
        }

        // hisSum 내용을 histogram.bin에 저장
        hisfp = fopen("histogram.bin", "wb");
        if (hisfp == NULL)
        {
            fprintf(stderr, "파일 열기 실패\n");
            exit(1);
        }
        fwrite(hisSum, 1, sizeof(hisSum), hisfp);
        fclose(hisfp);

        gettimeofday(&eTime, NULL); // 종료시간 저장

        // 실행시간(하나의 파일을 읽어서 histogram.bin에 저장하기까지) 출력
        runTime[i - sNum] = get_subtime(&sTime, &eTime);
        printf("data%d.bin 파일 수행시간 : %lds %dus\n", i, runTime[i - sNum].tv_sec, runTime[i - sNum].tv_usec);

        totalTime = get_sumtime(&totalTime, &runTime[i - sNum]); // 총 수행시간에 더하기
    }

    avgTime = get_avgtime(&totalTime, count);              // 평균
    stdDevTime = get_stddevtime(runTime, &avgTime, count); // 표준편차

    printf("%d개 파일 수행시간 합 : %lds %dus\n", count, totalTime.tv_sec, totalTime.tv_usec);
    printf("%d개 파일 수행시간 평균 : %lds %dus\n", count, avgTime.tv_sec, avgTime.tv_usec);
    printf("%d개 파일 수행시간 표준편차 : %lds %dus\n", count, stdDevTime.tv_sec, stdDevTime.tv_usec);

    return 0;
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

// 시간의 합을 timeval 구조체로 반환
timeval get_sumtime(timeval *time1, timeval *time2)
{
    timeval sumTime;

    sumTime.tv_sec = time1->tv_sec + time2->tv_sec;
    sumTime.tv_usec = time1->tv_usec + time2->tv_usec;

    while (sumTime.tv_usec >= MAX_USEC)
    {
        sumTime.tv_sec++;
        sumTime.tv_usec -= MAX_USEC;
    }

    return sumTime;
}

// 시간의 평균을 timeval 구조체로 반환
timeval get_avgtime(timeval *time, const int count)
{
    timeval avgTime = (timeval){0};

    // usec로 변환 후 평균 계산
    avgTime.tv_usec = (time->tv_sec * MAX_USEC + time->tv_usec) / count;

    while (avgTime.tv_usec >= MAX_USEC)
    {
        avgTime.tv_sec++;
        avgTime.tv_usec -= MAX_USEC;
    }

    return avgTime;
}

// 시간의 곱을 timeval 구조체로 반환
timeval get_multitime(timeval *time1, timeval *time2)
{
    timeval multiTime = (timeval){0};

    // usec으로 변환 후 곱셈하여 sec로 변환할 수 있도록 소숫점 자리를 이동하는 조건문
    if (time1->tv_sec > 0 && time2->tv_sec > 0)
    {
        multiTime.tv_usec = (time1->tv_sec * MAX_USEC + time1->tv_usec) * (time2->tv_sec * MAX_USEC + time2->tv_usec) / MAX_USEC;
    }
    else if (time1->tv_sec > 0 || time2->tv_sec > 0)
    {
        multiTime.tv_usec = (time1->tv_sec * MAX_USEC + time1->tv_usec) * (time2->tv_sec * MAX_USEC + time2->tv_usec) / get_sqrt(MAX_USEC);
    }
    else
    {
        multiTime.tv_usec = time1->tv_usec * time2->tv_usec;
    }

    while (multiTime.tv_usec >= MAX_USEC)
    {
        multiTime.tv_sec++;
        multiTime.tv_usec -= MAX_USEC;
    }

    return multiTime;
}

// 시간의 표준편차를 timeval 구조체로 반환
timeval get_stddevtime(timeval time[], timeval *avg, const int count)
{
    timeval devTime = (timeval){0};          // 시간편차(파일별 수행시간 - 평균 수행시간)
    timeval sqrDevTime = (timeval){0};       // 시간편차의 제곱
    timeval sumSqrDevTime = (timeval){0};    // 시간편차의 제곱의 합
    timeval avgSumSqrDevTime = (timeval){0}; // 시간편차의 제곱의 합의 평균
    timeval stdDevTime = (timeval){0};       // 표준편차(시간편차 제곱의 합의 평균의 제곱근)

    for (int i = 0; i < count; i++)
    {
        devTime = get_subtime(&time[i], avg);                     // 편차 구하기
        sqrDevTime = get_multitime(&devTime, &devTime);           // 편차의 제곱 구하기
        sumSqrDevTime = get_sumtime(&sumSqrDevTime, &sqrDevTime); // 편차 제곱의 합 구하기
    }

    avgSumSqrDevTime = get_avgtime(&sumSqrDevTime, count); // 분산 구하기

    stdDevTime.tv_usec = (long int)get_sqrt(avgSumSqrDevTime.tv_sec * MAX_USEC + avgSumSqrDevTime.tv_usec); // 표준편차 구하기

    while (stdDevTime.tv_usec >= MAX_USEC)
    {
        stdDevTime.tv_sec++;
        stdDevTime.tv_usec -= MAX_USEC;
    }

    return stdDevTime;
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

// 뉴튼-랩슨법을 이용한 제곱근 근삿값
// math.h는 gcc 컴파일러에 따로 연결해주어야 해서 따로 만듬
// 출처 : https://codeng.tistory.com/9
double get_sqrt(long int num)
{
    const int REPEAT = 18; // 16~20 정도 반복하면 상당히 정확한 값을 얻을 수 있다.
    double tmp = (double)num;
    double res = (double)num;

    for (int k = 0; k < REPEAT; k++)
    {
        if (res < 1.0)
            break;
        res = (res * res + tmp) / (2.0 * res);
    }

    return res;
}