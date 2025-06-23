#ifndef RequestWininetH
#define RequestWininetH

#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEM_BUFFER_SIZE 10

typedef struct
{
    LPSTR headerSend;
    LPSTR headerReceive;
    LPSTR message;
    long messageLength;
} HTTPRequest;

typedef struct
{
    unsigned char *buffer;
    unsigned char *position;
    size_t size;
} MemBuffer;

class Request
{
public:
    Request();
    virtual ~Request();

    void MemBufferCreate(MemBuffer *b);
    void MemBufferAddBuffer(MemBuffer *b, unsigned char *buffer, size_t size);

    int SendHTTP(LPCSTR url, BYTE *post, DWORD postLength, HTTPRequest *req, char *sid, long *httpcode);
    int SendRequest(bool IsPost, LPCSTR url, char *pszHeaderSend, DWORD HeaderSendLength, HTTPRequest *Ret, char *sid, long *httpcode);
};

#endif