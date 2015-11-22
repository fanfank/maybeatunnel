/******************************************
 *
 * 2015 reetsee.com
 *
 ******************************************/

/**
 * @file   cmd.c
 * @author xuruiqi
 * @date   2015-11-14 15:42:00
 * @brief  use to parse command line options
 **/

#ifndef CMD_C
#define CMD_C

#include <getopt.h>
#include <unistd.h>

#include "rts_str.c" //TODO(xuruiqi) finish rts_str.c

typedef struct global_conf_s global_conf_t;
struct global_conf_s {
    rts_str_t* type;
    rts_str_t* listen_host;
    rts_str_t* listen_port;
    rts_str_t* target_host;
    rts_str_t* target_port;
    rts_str_t* secret_key;
    rts_str_t* cryption_method;
    rts_str_t* config_path;
    int32_t worker_num: 16;
    int32_t max_connection_num: 16;
    int32_t downstream_timeout: 16;
    int32_t upstream_timeout: 16;
    int32_t debug: 1;
};
global_conf_t g_conf;

int32_t parse_cmd_opt(int32_t argc, char** argv) {
    int32_t check_global_conf_res = init_global_conf();
    static struct option long_options[] = {
        {"help",   no_argument,       NULL, 'h'},
        {"config", required_argument, NULL, 'c'},
        {"type",   required_argument, NULL, 't'},
        {"debug",  no_argument,       &(g_conf.debug), 1},
        {"listen_host", required_argument, NULL, 0},
        {"listen_port", required_argument, NULL, 0},
        {"target_host", required_argument, NULL, 0},
        {"target_port", required_argument, NULL, 0},
        {"worker_num",  required_argument, NULL, 0},
        {"secret_key",  required_argument, NULL, 0},
        {"cryption_method",    required_argument, NULL, 0},
        {"max_connection_num", required_argument, NULL, 0},
        {"downstream_timeout", required_argument, NULL, 0},
        {"upstream_timeout",   required_argument, NULL, 0},
        {NULL, 0, NULL, 0}
    };

    int32_t option_index = -1;

    while ((opt = getopt_long(
                argc, argv, "hc:t:", long_options, &option_index)
            ) != -1) {
        switch (opt) {
        case 'c':
            rts_str_append(g_conf.config_path, optarg, strlen(optarg));
            break;

        case 't':
            rts_str_append(g_conf.type, optarg, strlen(optarg));
            break;
            
        case 'h':
            //TODO(xuruiqi) print usage
            break;

        case 0:
            option_name = long_options[option_index].name;
            CHK_N_SET_STR_OPTION(option_name, listen_host, 
                    g_conf, optarg);
            CHK_N_SET_STR_OPTION(option_name, listen_port, 
                    g_conf, optarg);
            CHK_N_SET_STR_OPTION(option_name, target_host, 
                    g_conf, optarg);
            CHK_N_SET_STR_OPTION(option_name, target_port, 
                    g_conf, optarg);
            CHK_N_SET_STR_OPTION(option_name, secret_key, 
                    g_conf, optarg);
            CHK_N_SET_STR_OPTION(option_name, cryption_method, 
                    g_conf, optarg);
            CHK_N_SET_INT_OPTION(option_name, worker_num, 
                    g_conf, optarg);
            CHK_N_SET_INT_OPTION(option_name, max_connection_num, 
                    g_conf, optarg);
            CHK_N_SET_INT_OPTION(option_name, downstream_timeout, 
                    g_conf, optarg);
            CHK_N_SET_INT_OPTION(option_name, upstream_timeout, 
                    g_conf, optarg);
            break;

        case '?':
            //TODO(xuruiqi) print debug information
            break;
            
        default:
            //TODO(xuruiqi) print debug information
            break;
        }
    }

    return check_global_conf();
} //function parse_cmd_opt

int32_t check_global_conf() {
    //check type
    valid_types = {"client", "server", NULL};
    CHECK_RES(
        str_in_array(g_conf.type->buf, valid_types),
        FATAL, true, -1,
        "invalid type:[%s]\n", g_conf.type
    );

    //check cryption_method
    valid_cryption_method = {"aes256cbc", "rc4"};
    CHECK_RES(
        str_in_array(g_conf.cryption_method->buf, valid_cryption_method),
        FATAL, true, -2,
        "invalid cryption_method:[%s]\n", g_conf.cryption_method
    );

    //check worker_num
    CHECK_RES(
        (g_conf.worker_num <= 0 || g_conf.worker_num > 128),
        FATAL, true, -3,
        "invalid worker_num:[%d], too small or too large\n", g_conf.worker_num
    );

    //check max_connection_num
    CHECK_RES(
        (g_conf.max_connection_num <= 0),        
        FATAL, true, -4,
        "invalid max_connection_num:[%d], can not be <= 0\n", 
        g_conf.max_connection_num
    );

    //check downstream_timeout
    CHECK_RES(
        (g_conf.downstream_timeout <= 0),      
        FATAL, true, -5,
        "invalid downstream_timeout:[%d], can not be <= 0\n",
        g_conf.downstream_timeout
    );

    //check upstream_timeout
    CHECK_RES(
        (g_conf.upstream_timeout <= 0),      
        FATAL, true, -6,
        "invalid downstream_timeout:[%d], can not be <= 0\n",
        g_conf.upstream_timeout
    );

    return 0;
} //function check_global_conf

int32_t init_global_conf() {
    g_conf = {
        rts_buf_init(16), rts_buf_init(16), rts_buf_init(8), 
        rts_buf_init(16), rts_buf_init(8), rts_buf_init(32),
        rts_buf_init(16), rts_buf_init(32),
        1, 1000, 20, 20,
        0
    }
    g_conf.listen_host->append("0.0.0.0");
    g_conf.listen_port->append("8888");
    g_conf.cryption_method->append("aes256cbc");

    return 0;
} //function init_global_conf

#endif //CMD_C
