#ifndef RTS_BUF_C
#define RTS_BUF_C

#include <stdlib.h>

typedef struct rts_buf_s rts_buf_t;
struct rts_buf_s {
    char* buf;
    size_t size;
    size_t capacity;
};

rts_buf_t* rts_buf_init(size_t capacity) {
    if (capacity <= 0) {
        return NULL;
    }

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
        fprintf(stderr, "re-alloc occurs! new cap = [%d]\n", new_cap);
        char* new_buf = (char*)malloc(new_cap);
        if (new_buf == NULL) {
            return -3;
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

#endif //RTS_BUF_C
