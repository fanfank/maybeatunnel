/******************************************
 *
 * 2015 reetsee.com
 *
 ******************************************/

/**
 * @file   main.c
 * @author xuruiqi
 * @date   2015-11-14 15:42:00
 * @brief  entrance file
 **/

#include <inttypes.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "uv.h"

#include "conf.h"
#include "cmd.c"
#include "crypt.c"
#include "define.h"
#include "log.h"
#include "worker.h"

extern global_conf_t g_conf;
pid_t parent_pid;
pid_t cur_pid;
pid_t* g_pids;
uv_loop_t* loop;

int32_t main(int32_t argc, char** argv) {
    // init common parameters
    int32_t i, j, k;

    // TODO parse command line options
    int32_t parse_cmd_opt_res = parse_cmd_opt(argc, argv);
    CHECK_RES(parse_cmd_opt_res, FATAL, true, -1,
            "parse_cmd_opt failed, res=[%d]", parse_cmd_opt_res);

    // TODO init encryption/decryption method
    cryption_t* crpt = init_cryption(g_conf.cryption_method);
    CHECK_RES(crpt == NULL, FATAL, true, -2,
            "init_cryption failed, res=[%d]", init_cryption_res);

    // prepare to fork children
    g_pids = (pid_t*)malloc(g_conf.worker_num * sizeof(pid_t));
    CHECK_RES((g_pids == NULL), FATAL, true, -3,
            "malloc for g_pids failed");
    for (i = 0; i < worker_num; ++i) {
        g_pids[i] = -1;
    }

    parent_pid = getpid();
    cur_pid    = getpid();

    loop = uv_default_loop();

    uv_tcp_t server;
    uv_tcp_init(loop, &server);

    struct sockaddr_in addr;
    uv_ip4_addr(g_conf.listen_host, g_conf.listen_port, &addr);
    uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);

    int32_t uv_listen_res = uv_listen(
        (uv_stream_t*)&server, 
        g_conf.max_connection_num,
        on_new_downstream_connection
    );

    //TODO(xuruiqi) write spawn_workers
    int32_t spawn_workers_res = spawn_workers(g_conf.worker_num);
    CHECK_RES(spawn_workers_res, FATAL, true, -4,
            "spawn_workers failed, res=[%d]", spawn_workers_res);
    if (cur_pid == parent_pid) {
        // close master's listening event
        uv_close((uv_handle_t*)&server, NULL);

        int32_t join_res = 0;
        for (i = 0; i < g_conf.worker_num; ++i) {
            if (g_pids[i] <= 0) {
                continue;
            }
            join_res = waitpid(g_pids[i], NULL, 0);
            if (join_res == -1) {
                LOG_ERROR("Wait for pid %d failed\n", g_pids[i]);
            } else {
                LOG_INFO("Child process %d finished\n", g_pids[i]);
            }
        }
    } else {
        return uv_run(loop, UV_RUN_DEFAULT);
    }
    
    return 0;
}
