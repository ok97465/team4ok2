#include "Request_wininet.h"

// Ensure wininet.lib is linked in the build system or project settings.

Request::Request() {}

Request::~Request() {}

void Request::MemBufferCreate(MemBuffer *b)
{
    b->size = MEM_BUFFER_SIZE;
    b->buffer = (unsigned char *)malloc(b->size);
    b->position = b->buffer;
}

void Request::MemBufferAddBuffer(MemBuffer *b, unsigned char *buffer, size_t size)
{
    size_t used = b->position - b->buffer;
    if (used + size >= b->size)
    {
        b->size = used + size + MEM_BUFFER_SIZE;
        b->buffer = (unsigned char *)realloc(b->buffer, b->size);
        b->position = b->buffer + used;
    }
    memcpy(b->position, buffer, size);
    b->position += size;
}

int Request::SendHTTP(LPCSTR url, BYTE *post, DWORD postLength, HTTPRequest *req, char *sid, long *httpcode)
{
    printf("[%s] using wininet", __func__);

    HINTERNET hInternet = InternetOpenA("gefetch", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) return 1;

    HINTERNET hFile = InternetOpenUrlA(
        hInternet,
        url,
        NULL,
        0,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE,
        0
    );
    if (!hFile)
    {
        InternetCloseHandle(hInternet);
        return 1;
    }

    // HTTP 응답 코드 얻기
    DWORD statusCode = 0, len = sizeof(statusCode);
    if (HttpQueryInfoA(hFile, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &len, NULL))
    {
        if (httpcode) *httpcode = statusCode;
    }
    else
    {
        if (httpcode) *httpcode = 0;
    }

    // 헤더 수신 (간단히 구현)
    req->headerReceive = NULL;
    req->headerSend = NULL;

    // 데이터 읽기
    MemBuffer messageBuffer;
    MemBufferCreate(&messageBuffer);
    char temp[4096];
    DWORD bytesRead;
    while (InternetReadFile(hFile, temp, sizeof(temp), &bytesRead) && bytesRead > 0)
    {
        MemBufferAddBuffer(&messageBuffer, (unsigned char *)temp, bytesRead);
    }
    if (messageBuffer.position && messageBuffer.position > messageBuffer.buffer)
    {
        size_t msgLen = messageBuffer.position - messageBuffer.buffer;
        req->message = (char *)malloc(msgLen + 1);
        memcpy(req->message, messageBuffer.buffer, msgLen);
        req->message[msgLen] = 0;
        req->messageLength = (long)msgLen;
    }
    else
    {
        req->message = NULL;
        req->messageLength = 0;
    }
    free(messageBuffer.buffer);

    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);

    return 0;
}

int Request::SendRequest(bool IsPost, LPCSTR url, char *pszHeaderSend, DWORD HeaderSendLength, HTTPRequest *Ret, char *sid, long *httpcode)
{
    // POST 미지원 (GET만 지원)
    (void)IsPost;
    (void)pszHeaderSend;
    (void)HeaderSendLength;
    (void)sid;
    printf("[%s] using wininet", __func__);
    return SendHTTP(url, NULL, 0, Ret, NULL, httpcode);
}