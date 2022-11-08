/**
 * @file sma.h
 *
 */
#include <stddef.h>
#include <stdbool.h>

#define SMA_FD_NAME  "lvgl-wayland"
typedef enum {
   SMA_BG_0 = 0,
   SMA_BG_1,
   SMA_BG_2,
   SMA_BG_3,
   SMA_BG_4,
   SMA_BG_5,
   SMA_BG_6,
   SMA_BG_7,
   SMA_BG_8,
   SMA_BG_9,
   SMA_NUM_BG
} sma_buffer_group_t;

#define SMA_TAG_POOL(p, v) (((struct sma_pool_properties *)(p))->tag = (v))
#define SMA_POOL_PROPERTIES(p) ((const struct sma_pool_properties *)(p))
#define SMA_TAG_BUFFER(b, v) (((struct sma_buffer_properties *)(b))->tag = (v))
#define SMA_BUFFER_PROPERTIES(b) ((const struct sma_buffer_properties *)(b))

typedef void sma_pool_t;
typedef void sma_buffer_t;

struct sma_events {
   void *ctx;
   bool (*new_pool)(void *ctx, sma_pool_t *pool);
   void (*expand_pool)(void *ctx, sma_pool_t *pool);
   void (*free_pool)(void *ctx, sma_pool_t *pool);
   bool (*new_buffer)(void *ctx, sma_buffer_t *buf);
   void (*free_buffer)(void *ctx, sma_buffer_t *buf);
};

struct sma_pool_properties {
   void *tag;
   size_t size;
   int fd;
};

struct sma_buffer_properties {
   void *tag;
   sma_buffer_group_t group;
   sma_pool_t *const pool;
   size_t offset;
};

void sma_init(struct sma_events *evs);
void sma_deinit(void);
void sma_setctx(void *ctx);
void sma_resize(sma_buffer_group_t grp, size_t sz);
sma_buffer_t *sma_alloc(sma_buffer_group_t grp);
void *sma_map(sma_buffer_t *buf);
void sma_free(sma_buffer_t *buf);
