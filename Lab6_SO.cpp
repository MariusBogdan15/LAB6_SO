#include <windows.h> 
#include <stdio.h> 
#include <tchar.h>
#include <strsafe.h>
#include <iostream>
using namespace std;
#define BUFSIZE 512

DWORD WINAPI InstanceThread(LPVOID);
VOID GetAnswerToRequest(LPTSTR, LPTSTR, LPDWORD);

int _tmain(VOID)
{
    BOOL   fConnected = FALSE;
    DWORD  dwThreadId = 0;
    HANDLE hPipe = INVALID_HANDLE_VALUE, hThread = NULL;
    LPCTSTR lpszPipename = TEXT("\\\\.\\pipe\\mynamedpipe");

    for (;;)
    {
        _tprintf(TEXT("\nPipe Server: Main thread awaiting client connection on %s\n"), lpszPipename);
        hPipe = CreateNamedPipe(
            lpszPipename,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            BUFSIZE,
            BUFSIZE,
            0,
            NULL);

        if (hPipe == INVALID_HANDLE_VALUE)
        {
            _tprintf(TEXT("CreateNamedPipe failed, GLE=%d.\n"), GetLastError());
            return -1;
        }

        fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (fConnected)
        {
            printf("Client connected, creating a processing thread.\n");
            hThread = CreateThread(
                NULL,
                0,
                InstanceThread,
                (LPVOID)hPipe,
                0,
                &dwThreadId);

            if (hThread == NULL)
            {
                _tprintf(TEXT("CreateThread failed, GLE=%d.\n"), GetLastError());
                return -1;
            }
            else CloseHandle(hThread);
        }
        else
            CloseHandle(hPipe);
    }

    return 0;
}

DWORD WINAPI InstanceThread(LPVOID lpvParam)
{
    HANDLE hHeap = GetProcessHeap();
    TCHAR* pchRequest = (TCHAR*)HeapAlloc(hHeap, 0, BUFSIZE * sizeof(TCHAR));
    TCHAR* pchReply = (TCHAR*)HeapAlloc(hHeap, 0, BUFSIZE * sizeof(TCHAR));

    DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0;
    BOOL fSuccess = FALSE;
    HANDLE hPipe = NULL;

    if (lpvParam == NULL)
    {
        if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
        if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
        return (DWORD)-1;
    }

    if (pchRequest == NULL || pchReply == NULL)
    {
        if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
        if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
        return (DWORD)-1;
    }

    printf("InstanceThread created, receiving and processing messages.\n");

    hPipe = (HANDLE)lpvParam;

    while (1)
    {
        fSuccess = ReadFile(hPipe, pchRequest, BUFSIZE * sizeof(TCHAR), &cbBytesRead, NULL);

        if (!fSuccess || cbBytesRead == 0)
        {
            if (GetLastError() == ERROR_BROKEN_PIPE)
            {
                _tprintf(TEXT("InstanceThread: client disconnected.\n"));
            }
            else
            {
                _tprintf(TEXT("InstanceThread ReadFile failed, GLE=%d.\n"), GetLastError());
            }
            break;
        }

        GetAnswerToRequest(pchRequest, pchReply, &cbReplyBytes);

        fSuccess = WriteFile(hPipe, pchReply, cbReplyBytes, &cbWritten, NULL);

        if (!fSuccess || cbReplyBytes != cbWritten)
        {
            _tprintf(TEXT("InstanceThread WriteFile failed, GLE=%d.\n"), GetLastError());
            break;
        }
    }

    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);

    HeapFree(hHeap, 0, pchRequest);
    HeapFree(hHeap, 0, pchReply);

    printf("InstanceThread exiting.\n");
    return 1;
}

VOID GetAnswerToRequest(LPTSTR pchRequest, LPTSTR pchReply, LPDWORD pchBytes)
{
    _tprintf(TEXT("Client Request String:\"%s\"\n"), pchRequest);

    if (FAILED(StringCchCopy(pchReply, BUFSIZE, TEXT("default answer from server"))))
    {
        *pchBytes = 0;
        pchReply[0] = 0;
        return;
    }
    *pchBytes = (lstrlen(pchReply) + 1) * sizeof(TCHAR);
}

int main(void)
{
    CHAR chBuf[BUFSIZE];
    DWORD dwRead, dwWritten;
    HANDLE hStdin, hStdout;
    BOOL bSuccess;

    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    hStdin = GetStdHandle(STD_INPUT_HANDLE);

    if ((hStdout == INVALID_HANDLE_VALUE) || (hStdin == INVALID_HANDLE_VALUE))
        ExitProcess(1);

    for (int i = 1; i <= 10000; i++)
    {
        bool ok = 1;
        for (int j = 2; j <= i / 2; j++)
            if (i % j == 0)
                ok = 0;
        if (ok)
            cout << i << " ";
    }

    for (;;)
    {
        bSuccess = ReadFile(hStdin, chBuf, BUFSIZE, &dwRead, NULL);

        if (!bSuccess || dwRead == 0)
            break;

        bSuccess = WriteFile(hStdout, chBuf, dwRead, &dwWritten, NULL);

        if (!bSuccess)
            break;
    }
    return 0;
}
