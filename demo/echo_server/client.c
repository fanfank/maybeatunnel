#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uv.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888

uv_loop_t* loop;

void on_connect(uv_connect_t* connect, int status);
void after_write(uv_write_t* req, int status);
void after_read(uv_stream_t* resp, ssize_t nread, const uv_buf_t* buf);
void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = (char*)malloc(suggested_size);
    buf->len  = suggested_size;
}

typedef struct rts_buf_s rts_buf_t;
struct rts_buf_s {
    char* buf;
    size_t size;
    size_t capacity;
};
rts_buf_t* rts_buf_init(size_t capacity) {
    char* buf = (char*)malloc(capacity);
    if (buf == NULL) {
        return NULL;
    }

    rts_buf_t* rts_buf = (rts_buf_t*)malloc(sizeof(rts_buf_t));
    rts_buf->buf      = buf;
    rts_buf->size     = 0;
    rts_buf->capacity = capacity;

    return rts_buf;
}
void rts_buf_free(rts_buf_t* rts_buf) {
    if (rts_buf != NULL && rts_buf->buf != NULL) {
        free(rts_buf->buf);
    }
    free(rts_buf);
}

int rts_buf_append(rts_buf_t* rts_buf, 
        char* content, 
        size_t content_len) {
    if (content == NULL) {
        return -1;
    }
    
    size_t alloc_len = content_len + 1; //for '\0'

    if (rts_buf->capacity - rts_buf->size < alloc_len) {
        int new_cap = rts_buf->size + alloc_len - rts_buf->capacity 
                >= rts_buf->capacity ? rts_buf->size + alloc_len :
                    rts_buf->capacity * 2;
        char* new_buf = (char*)malloc(new_cap);
        if (new_buf == NULL) {
            return -2;
        }
        strncpy(new_buf, rts_buf->buf, rts_buf->size);
        rts_buf->capacity = new_cap;
        free(rts_buf->buf);
        rts_buf->buf = new_buf;
    }

    strncpy(rts_buf->buf + rts_buf->size, content, content_len);
    rts_buf->size += content_len;

    return 0;
}

int main() {
    loop = uv_default_loop();

    struct sockaddr_in dest;
    uv_ip4_addr(SERVER_IP, SERVER_PORT, &dest);

    uv_tcp_t     client[100];
    uv_connect_t connect[100];
	int i = 0;
    for (; i < 100; ++i) {
        client[i].data = (void*)rts_buf_init(16);
        uv_tcp_init(loop, &client[i]);
        uv_tcp_connect(&connect[i], &client[i], (struct sockaddr*)&dest, 
                on_connect);
    } 
    return uv_run(loop, UV_RUN_DEFAULT);
}

void on_connect(uv_connect_t* connect, int status) {
    if (status < 0) {
        fprintf(stderr, "connect error %s\n", uv_strerror(status));
        return;
    } else {
        fprintf(stdout, "connect success\n");
    }

    uv_read_start(connect->handle, alloc_buffer, after_read);

    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    uv_buf_t wrbuf  = uv_buf_init("Hello Wall!", strlen("Hello Wall!"));
    uv_write(req, connect->handle, &wrbuf, 1, after_write);
}

void after_write(uv_write_t* req, int status) {
    if (status) {
        fprintf(stderr, "write error [%s]\n", uv_strerror(status));
    }

    if (req->handle == req->send_handle && req->handle != NULL) {
        fprintf(stdout, "handle and send_handle are same\n");
    }
    fprintf(stdout, "content sent\n");

    free(req);
}

void after_read(uv_stream_t* resp, ssize_t nread, const uv_buf_t* buf) {
    rts_buf_t* rts_buf = (rts_buf_t*)resp->data;

    fprintf(stdout, "after_read called, nread=%zd\n", nread);
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        } else {
            rts_buf->buf[rts_buf->size] = '\0';
            fprintf(stdout, "recv: %s\n", rts_buf->buf);
            rts_buf->size = 0;
        }
        uv_close((uv_handle_t*)resp, NULL);
    } else if (nread > 0) {
        if (rts_buf_append(rts_buf, buf->base, nread) != 0) {
            fprintf(stderr, "append buf failed\n");
        }
    } else {
        fprintf(stderr, "nread is 0\n");
        rts_buf->buf[rts_buf->size] = '\0';
        fprintf(stdout, "recv: %s\n", rts_buf->buf);
        rts_buf->size = 0;
    }

    if (buf->base) {
        free(buf->base);
    }
}
