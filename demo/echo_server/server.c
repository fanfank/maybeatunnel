#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "uv.h"

#define DEFAULT_PORT 8888
#define DEFAULT_BACKLOG 128
#define DEFAULT_NWORKERS 5

pid_t parent_pid;
pid_t cur_pid;
pid_t g_pids[DEFAULT_NWORKERS];

uv_loop_t* loop;

int spawn_processes(int nworkers);
void on_new_connection(uv_stream_t* server, int status);
void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
void echo_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf);
void echo_write(uv_write_t* req, int status);

int main() {
    parent_pid = getpid();
    cur_pid    = getpid();

    loop = uv_default_loop();

    uv_tcp_t server;
    uv_tcp_init(loop, &server);

    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", DEFAULT_PORT, &addr);
    uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);

    int32_t r = uv_listen((uv_stream_t*)&server, DEFAULT_BACKLOG, 
            on_new_connection);
    if (r) {
        fprintf(stderr, "Listen error %s\n", uv_strerror(r));
        return 1;
    }

    spawn_processes(DEFAULT_NWORKERS);

    if (cur_pid == parent_pid) {
        uv_close((uv_handle_t*)&server, NULL);
        int ret;
        int i;
        for (i = 0; i < DEFAULT_NWORKERS; ++i) {
            ret = waitpid(g_pids[i], NULL, 0);
            if (ret == -1) {
                fprintf(stderr, "Wait for pid %d failed\n", g_pids[i]);
            } else if (ret == g_pids[i]) {
                fprintf(stdout, "Child process %d finished\n", g_pids[i]);
            }
        }
        return 0;
    } else {
        //TODO 
        return uv_run(loop, UV_RUN_DEFAULT);
    }
}

int spawn_processes(int nworkers) {
    int i = 0;
    for (; i < nworkers; ++i) {
        g_pids[i] = fork();

        switch (g_pids[i]) {
        case -1:
            printf("error forking the %dth child\n", i);
            return -1;
            break;

        case 0:
            cur_pid = getpid();
            break;
            
        default:
            break;
        }

        if (cur_pid != parent_pid) {
            break;
        }
    }
    return 0;
}

void on_new_connection(uv_stream_t* server, int status) {
    fprintf(stdout, "child [%u] handling new connection...\n", cur_pid);
    
    if (status < 0) {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        return;
    }

    uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, client);
    if (uv_accept(server, (uv_stream_t*)client) == 0) {
        uv_read_start((uv_stream_t*)client, alloc_buffer, echo_read);
    } else {
        uv_close((uv_handle_t*)client, NULL);
    }
}

void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    suggested_size += 1;
    buf->base = (char*)malloc(suggested_size);
    buf->len  = suggested_size;
}

void echo_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
    fprintf(stderr, "echo_read called\n");
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "Read error [%s] in process [%u]\n",
                    uv_err_name(nread), cur_pid);
        }
        uv_close((uv_handle_t*)client, NULL);
    } else if (nread > 0) {
        uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
        uv_buf_t wrbuf  = uv_buf_init(buf->base, nread);
        uv_write(req, client, &wrbuf, 1, echo_write);
        buf->base[nread] = '\0';
        fprintf(stdout, "server received %s\n", buf->base);
    }

    if (buf->base) {
        free(buf->base);
    }
}

void echo_write(uv_write_t* req, int status) {
    if (status) {
        fprintf(stderr, "Write error [%s] in process [%u]\n",
                uv_strerror(status), cur_pid);
    }
    fprintf(stderr, "echo_write called\n");
    free(req);
}
