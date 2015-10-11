#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uv.h"

#define DEFAULT_PORT 8888
#define DEFAULT_BACKLOG 128
#define DEFAULT_NWORKERS 5

pid_t parent_pid = getpid();
pid_t cur_pid    = getpid();
pid_t g_pids[DEFAULT_NWORKERS];

int spawn_processes(int nworkers);

int main() {
    uv_loop_t* loop = uv_default_loop();

    uv_tcp_t server;
    uv_tcp_init(loop, &server);

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
    }

    //TODO

    return uv_run(loop, UV_RUN_DEFAULT);
}

int spawn_processes(int nworkers) {
    for (int32_t i = 0; i < nworkers; ++i) {
        g_pids[i] = fork();

        switch (g_pids[i]) {
        case -1:
            printf("error forking the %dth child\n", i);
            break;

        case 0:
            cur_pid = get_pid();
            break;
            
        default:
            break;
        }

        if (cur_pid != parent_pid) {
            break;
        }
    }
}
