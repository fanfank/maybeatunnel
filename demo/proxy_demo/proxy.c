//NOTE: this demo does not gurantee that there is no memory leak
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "uv.h"

#include "rts_buf.c"

#define DEFAULT_PORT 8888
#define DEFAULT_BACKLOG 128
#define DEFAULT_NWORKERS 5
#define UPSTREAM_HOST "127.0.0.1"
#define UPSTREAM_PORT "4444"

pid_t parent_pid;
pid_t cur_pid;
pid_t g_pids[DEFAULT_NWORKERS];

uv_loop_t* loop;

int spawn_processes(int nworkers);
void on_new_downstream_connection(uv_stream_t* server, int status);
void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
void downstream_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf);
void upstream_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf);
void after_upstream_write(uv_write_t* req, int status);
int init_upstream_connection(uv_stream_t* downstream);
void on_upstream_connect(uv_connect_t* connect, int status);
int init_callback_data(void* data);
int cleanup_callback_data(uv_stream_t* stream);
int transfer_stream_data(uv_stream_t* from, uv_stream_t* to, uv_write_cb wrcb);

typedef struct stream_conn_s stream_conn_t;
struct stream_conn_s {
    uv_stream_t* stream;
    unsigned ready: 1;
};

typedef struct callback_data_s callback_data_t;
struct callback_data_t {
    stream_conn_t* stream_conn;
    rts_buf_t* buf;
};

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
            on_new_downstream_connection);
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

int init_callback_data(callback_data_t* data) {
    data = (callback_data_t*)malloc(sizeof(callback_data_t));
    if (data == NULL) {
        return -1;
    }

    data->stram_conn = NULL;
    data->buf = NULL;
    return 0;
}

void on_new_downstream_connection(uv_stream_t* server, int status) {
    fprintf(stdout, "child [%u] handling new connection...\n", cur_pid);
    
    if (status < 0) {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        return;
    }

    uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    init_callback_data((callback_data_t*)client->data);
    uv_tcp_init(loop, client);
    if (uv_accept(server, (uv_stream_t*)client) == 0) {
        uv_read_start((uv_stream_t*)client, alloc_buffer, downstream_read);
    } else {
        cleanup_callback_data((uv_stream_t*)client);
        uv_close((uv_handle_t*)client, NULL);
    }
}

void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    suggested_size += 1;
    buf->base = (char*)malloc(suggested_size);
    buf->len  = suggested_size;
}

void downstream_read(uv_stream_t* client, ssize_t nread, 
        const uv_buf_t* buf) {

    fprintf(stderr, "downstream_read called\n");
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "downstream_read error [%s] in process [%u]\n",
                    uv_err_name(nread), cur_pid);
        }

        if (client->data->stream_conn) {
            cleanup_callback_data(client->data->stream_conn->stream);
            uv_close((uv_handle_t*)client->data->stream_conn->stream, NULL);
        }

        cleanup_callback_data(client);
        uv_close((uv_handle_t*)client, NULL);
    } else if (nread > 0) {
        buf->base[nread] = '\0';
        fprintf(stdout, "%s", buf->base);

        callback_data_t cb_data = (callback_data_t*)client->data;
        if (cb_data->stream_conn == NULL) {
            if (init_upstream_connection(client) != 0) {
                fprintf(stderr, "inti upstream connection error\n");
                cleanup_callback_data(client);
                uv_close((uv_handle_t*)client, NULL);
                if (buf->base) {
                    free(buf->base);
                }
                return;
            }
        }

        //简化设计，将内容先全部读到下游数据缓存中
        rts_buf_append(cb_data, buf->base, nread);

        if (cb_data->stream_conn->ready) {
            transfer_stream_data(client, client->data->stream_conn, 
                    after_upstream_write);
        }
    }

    if (buf->base) {
        free(buf->base);
    }
}

void after_upstream_write(uv_write_t* req, int status) {
    if (status) {
        fprintf(stderr, "write to upstream error [%s] in process [%u]\n",
                uv_strerror(status), cur_pid);
    }
    fprintf(stderr, "echo_write called\n");
    free(req);
}

int init_upstream_connection(uv_stream_t* downstream) {
    if (downstream == NULL || downstream->data != NULL) {
        return -1;
    }

    struct sockaddr_in dest;
    uv_ip4_addr(UPSTREAM_HOST, UPSTREAM_PORT, &dest);

    uv_tcp_t* upstream = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_connect_t* conn = (uv_connect_t*)malloc(sizeof(uv_connect_t));
    stream_conn_t* downstream_conn = 
            (stream_conn_t*)malloc(stream_conn_t);
    stream_conn_t* upstream_conn = 
            (stream_conn_t*)malloc(stream_conn_t);

    int init_cb_data_res = init_callback_data((callback_data_t*)upstream->data);

    if (!upstream || !conn || !downstream_conn || !upstream_conn 
            || init_cb_data_res != 0) {
        free(upstream);
        free(conn);
        free(downstream_conn);
        free(upstream_conn);
        return -2;
    }

    downstream_conn->stream = downstream;
    downstream_conn->ready = 1;

    upstream_conn->stream = upstream;
    upstream_conn->ready = 0;

    downstream->data->stream_conn = upstream_conn;
    upstream->data->stream_conn   = downstream_conn;
    
    uv_tcp_init(loop, upstream);
    uv_tcp_connect(conn, upstream, (struct sockaddr*)&dest,
            on_upstream_connect);

    return 0;
}

void on_upstream_connect(uv_connect_t* connect, int status) {

    stream_conn_t* upstream_conn   = connect->handle;
    stream_conn_t* downstream_conn = upstream_conn->data->stream_conn;

    if (status < 0) {
        fprintf(stderr, "on_upstream_connect error %s\n", uv_strerror(status));

        cleanup_callback_data(upstream_conn);

        //原本应该向下游发送503错误然后关闭连接
        //现在为了简化情况，直接关闭与下游的连接
        cleanup_callback_data(downstream_conn);
        uv_close((uv_handle_t*)downstream_conn, NULL);
    } else {
        fprintf(stdout, "connect to upstream success\n");
    }
    
    uv_read_start((uv_stream_t*)downstream_conn->stream, alloc_buffer, 
            upstream_read);

    downstream_conn->data->ready = 1;

    transfer_stream_data(downstream_conn, upstream_conn, after_upstream_write);

    free(connect);
}

int transfer_stream_data(uv_stream_t* from, uv_stream_t* to, 
        uv_write_cb wrcb) {

    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    uv_buf_t wrbuf  = uv_buf_init(from->data->buf->buf, from->data->buf->size);
    from->data->buf->size = 0;
    uv_write(req, to, &wrbuf, 1, wrcb);

    return 0;
}
void upstream_read(uv_stream_t* client, ssize_t nread, 
        const uv_buf_t* buf) {

    fprintf(stderr, "upstream_read called\n");
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "upstream_read error [%s] in process [%u]\n",
                    uv_err_name(nread), cur_pid);
        }

        cleanup_callback_data(client->data->stream_conn->stream);
        uv_close((uv_handle_t*)client->data->stream_conn->stream, NULL);

        cleanup_callback_data(client);
        uv_close((uv_handle_t*)client, NULL);
    } else if (nread > 0) {
        buf->base[nread] = '\0';
        fprintf(stdout, "%s", buf->base);

        callback_data_t cb_data = (callback_data_t*)client->data;

        //简化设计，将内容先全部读到上游数据缓存中
        rts_buf_append(cb_data, buf->base, nread);

        if (cb_data->stream_conn->ready) {
            transfer_stream_data(client, client->data->stream_conn, 
                    after_downstream_write);
        }
    }

    if (buf->base) {
        free(buf->base);
    }
}

void after_downstream_write(uv_write_t* req, int status) {
    if (status) {
        fprintf(stderr, "write to downstream error [%s] in process [%u]\n",
                uv_strerror(status), cur_pid);
    }
    fprintf(stderr, "echo_write called\n");
    free(req);
}

int cleanup_callback_data(uv_stream_t* stream) {
    if (stream->data == NULL) {
        return 0;
    }

    if (stream->data->buf) {
        rts_buf_free(stream->data->buf);
        stream->data->buf = NULL;
    }

    if (stream->data->stream_conn) {
        free(stream->data->stream_conn);
        stream->data->stream_conn = NULL;
    }

    free(stream->data);
    stream->data = NULL;

    return 0;
}
