/*
 * Copyright 2002-2020 Intel Corporation.
 * 
 * This software is provided to you as Sample Source Code as defined in the accompanying
 * End User License Agreement for the Intel(R) Software Development Products ("Agreement")
 * section 1.L.
 * 
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

extern int Add2(int,int);

int ThreadRoutine()
{
    int i = 0;
    for(i = 0; i < 1000; i++)
    {
        void * h =  malloc(13);
        if (h)
            free(h);
    }
    return 0;
}

int ThreadCreation()
{
    const unsigned long num_threads = 64;
    static HANDLE aThreads[64] = { 0 };
    unsigned long slot = 0;
    unsigned long thread_id = 0;
    unsigned long cnt_th = 0;
    unsigned long thread_ret = 0;
    
    fprintf(stderr, "creating %d threads \n", num_threads);
    
    for (cnt_th = 0; cnt_th < num_threads; cnt_th++)
    {
        aThreads[cnt_th] = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ThreadRoutine,0,0,(LPDWORD)&thread_id);
    }
    
    fprintf(stderr, "created %d threads \n", num_threads);
    while (cnt_th  > 0)
    {
        slot = WaitForMultipleObjects(cnt_th, aThreads, FALSE, INFINITE);
        GetExitCodeThread(aThreads[slot],&thread_ret);
        CloseHandle(aThreads[slot]);
        aThreads[slot] = aThreads[cnt_th-1];
        cnt_th--;
        fprintf(stderr, "addit =%d\n", Add2(cnt_th, cnt_th));
    }
    fprintf(stderr, "all %d threads terminated\n", num_threads);
    fflush(stderr);
    return 1;
}

int main()
{
    ThreadCreation();
    return 0;
}

