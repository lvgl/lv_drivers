/**
 * @file sma.c
 *
 */
#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "sma.h"

#define MAX_NAME_ATTEMPTS (5)

/* Ungrouped buffer value (i.e. buffer only in pool) */
#define BUFFER_UNGROUPED (SMA_NUM_BG)
 
struct sma_pool {
   struct sma_pool_properties props;
   struct sma_buffer *allocd;
   void *map;
   size_t map_size;
   bool map_outdated;
};

struct sma_buffer {
   struct sma_buffer_properties props;
   bool group_resized;
   struct sma_buffer *prev;
   struct sma_buffer *next;
   struct sma_buffer *prev_in_pool;
   struct sma_buffer *next_in_pool;
};

static struct sma_buffer *alloc_from_pool(sma_buffer_group_t grp);
static void free_to_pool(struct sma_buffer *buf);
static struct sma_pool *alloc_pool(void);
static void free_pool(struct sma_pool *pool);
static struct sma_buffer *alloc_buffer(struct sma_buffer *last, size_t offset);
static void free_buffer(struct sma_buffer *buf);

static struct {
   unsigned long page_sz;
   struct sma_events cbs;
   struct sma_pool *active;
   size_t active_used;
   struct {
      struct sma_buffer *unused;
      struct sma_buffer *inuse;
      size_t size;
   } group[SMA_NUM_BG];
} sma_instance;


void sma_init(struct sma_events *evs)
{
   sma_buffer_group_t bgi;

   memcpy(&sma_instance.cbs, evs, sizeof(struct sma_events));
   srand((unsigned int)clock());
   sma_instance.page_sz = (unsigned long)sysconf(_SC_PAGESIZE);

   /* Default buffer group size(s), in page size multiples */
   for (bgi = 0; bgi < SMA_NUM_BG; bgi++) {
      sma_instance.group[bgi].size = sma_instance.page_sz;
   }
}


void sma_deinit(void)
{
   sma_buffer_group_t bgi;
   struct sma_buffer *buf;
   struct sma_buffer *next_buf;

   /* Make no pool active */
   sma_instance.active = NULL;

   /* Return all buffers to their respective pools (when all buffers are
    * returned, and with no active pool, all pools will also be freed)
    */
   for (bgi = 0; bgi < SMA_NUM_BG; bgi++) {
      /* Return unused buffers */
      for (buf = sma_instance.group[bgi].unused; buf != NULL; buf = next_buf) {
         next_buf = buf->next;
         free_to_pool(buf);
      }

      /* Return buffers that are still in use (ideally this list should be
       * empty at this time)
       */
      for (buf = sma_instance.group[bgi].inuse; buf != NULL; buf = next_buf) {
         next_buf = buf->next;
         free_to_pool(buf);
      }

      sma_instance.group[bgi].unused = NULL;
      sma_instance.group[bgi].inuse = NULL;
   }
}


void sma_setctx(void *ctx)
{
   sma_instance.cbs.ctx = ctx;
}


void sma_resize(sma_buffer_group_t grp, size_t sz)
{
   struct sma_buffer *buf;
   struct sma_buffer *next_buf;

   /* Round allocation size up to a sysconf(_SC_PAGE_SIZE) boundary */
   sz = (((sz ? sz : 1) + sma_instance.page_sz - 1) / sma_instance.page_sz);
   sz *= sma_instance.page_sz;
   sma_instance.group[grp].size = sz;

   /* Return all unused buffers to pool (so they may be re-allocated at the new
    * size)
    */ 
   for (buf = sma_instance.group[grp].unused; buf != NULL; buf = next_buf) {
      next_buf = buf->next;
      free_to_pool(buf);
   }
   sma_instance.group[grp].unused = NULL;

   /* Mark all buffers in use to be freed to pool when possible */
   for (buf = sma_instance.group[grp].inuse; buf != NULL; buf = buf->next) {
      buf->group_resized = true;
   }
}


sma_buffer_t *sma_alloc(sma_buffer_group_t grp)
{
   struct sma_buffer *buf;

   if (sma_instance.group[grp].unused != NULL) {
      /* Unused buffer available, reuse it */
      buf = sma_instance.group[grp].unused;
      sma_instance.group[grp].unused = buf->next;
      if (buf->next != NULL) {
         buf->next->prev = NULL;
      }
      buf->next = NULL;
   } else {
      buf = alloc_from_pool(grp);
   }

   if (buf != NULL) {
      /* Add buffer to in-use list */
      buf->prev = NULL;
      buf->next = sma_instance.group[grp].inuse;
      sma_instance.group[grp].inuse = buf;
      if (buf->next != NULL) {
         buf->next->prev = buf;
      }
   }

   return buf;
}


void sma_free(sma_buffer_t *buf)
{
   struct sma_buffer *fbuf = buf;
   sma_buffer_group_t grp = fbuf->props.group;

   /* TODO: Determine max number of buffers to keep in unused list */

   /* Remove from in-use list */
   if (sma_instance.group[grp].inuse == fbuf) {
      sma_instance.group[grp].inuse = fbuf->next;
      if (fbuf->next != NULL) {
         fbuf->next->prev = NULL;
      }
   } else {
      fbuf->prev->next = fbuf->next;
      if (fbuf->next != NULL) {
         fbuf->next->prev = fbuf->prev;
      }
   }

   if (fbuf->group_resized) {
      /* Buffer group was resized while this buffer was in-use, thus it must be
       * returned to it's pool
       */
      fbuf->group_resized = false;
      free_to_pool(fbuf);
   } else {
      /* Move to unused list */
      fbuf->prev = NULL;
      fbuf->next = sma_instance.group[grp].unused;
      sma_instance.group[grp].unused = fbuf;
      if (fbuf->next != NULL) {
         fbuf->next->prev = fbuf;
      }
   }
}


void *sma_map(sma_buffer_t *buf)
{
   struct sma_buffer *mbuf = buf;
   struct sma_pool *pool = mbuf->props.pool;
   void *map = pool->map;

   if (pool->map_outdated) {
      /* Update mapping to current pool size */
      if (pool->map != NULL) {
         munmap(pool->map, pool->map_size);
      }

      map = mmap(NULL,
                 pool->props.size,
                 PROT_READ | PROT_WRITE,
                 MAP_SHARED,
                 pool->props.fd,
                 0);

      if (map == MAP_FAILED) {
         map = NULL;
         pool->map = NULL;
      } else {
         pool->map = map;
         pool->map_size = pool->props.size;
         pool->map_outdated = false;
      }
   }

   /* Calculate buffer mapping (from offset in pool_ */
   if (map != NULL) {
      map = (((char *)map) + mbuf->props.offset);
   }

   return map;
}


size_t calc_buffer_size(struct sma_buffer *buf)
{
   size_t buf_sz;
   struct sma_pool *buf_pool = buf->props.pool;

   if (buf->next_in_pool == NULL) {
      buf_sz = (buf_pool->props.size - buf->props.offset);
   } else {
      buf_sz = (buf->next_in_pool->props.offset - buf->props.offset);
   }

   return buf_sz;
}


struct sma_buffer *alloc_from_pool(sma_buffer_group_t grp)
{
   int ret;
   size_t buf_sz;
   struct sma_buffer *buf;
   size_t grp_sz = sma_instance.group[grp].size;
   struct sma_buffer *last = NULL;

   /* TODO: Determine when to allocate a new active pool (i.e. memory shrink) */

   if (sma_instance.active == NULL) {
      /* Allocate a new active pool */
      sma_instance.active = alloc_pool();
      sma_instance.active_used = 0;
   }

   if (sma_instance.active == NULL) {
      buf = NULL;
   } else {
      /* Search for a free buffer large enough for allocation */
      for (buf = sma_instance.active->allocd;
           buf != NULL;
           buf = buf->next_in_pool) {
         last = buf;
         if (buf->props.group == BUFFER_UNGROUPED) {
            buf_sz = calc_buffer_size(buf);
            if (buf_sz == grp_sz) {
               break;
            } else if (buf_sz > grp_sz) {
               if ((buf->next_in_pool != NULL) &&
                   (buf->next_in_pool->props.group == BUFFER_UNGROUPED)) {
                  /* Pull back next buffer to use unallocated size */
                  buf->next_in_pool->props.offset -= (buf_sz - grp_sz);
               } else {
                  /* Allocate another buffer to hold unallocated size */
                  alloc_buffer(buf, buf->props.offset + grp_sz);
               }

               break;
            }
         }
      }

      if (buf == NULL) {
         /* No buffer found to meet allocation size, expand pool */
         if ((last != NULL) &&
             (last->props.group == BUFFER_UNGROUPED)) {
            /* Use last free buffer */
            buf_sz = (grp_sz - buf_sz);
         } else {
            /* Allocate new buffer */
            buf_sz = grp_sz;
            if (last == NULL) {
               buf = alloc_buffer(NULL, 0);
            } else {
               buf = alloc_buffer(last, sma_instance.active->props.size);
            }
            last = buf;
         }

         if (last != NULL) {
            /* Expand pool backing memory */
            ret = ftruncate(sma_instance.active->props.fd,
                            sma_instance.active->props.size + buf_sz);
            if (ret) {
               if (buf != NULL) {
                  free_buffer(buf);
                  buf = NULL;
               }
            } else {
               sma_instance.active->props.size += buf_sz;
               sma_instance.active->map_outdated = true;
               buf = last;

               if (!(sma_instance.active->props.size - buf_sz)) {
                  /* Emit 'new pool' event */
                  if ((sma_instance.cbs.new_pool != NULL) &&
                      (sma_instance.cbs.new_pool(sma_instance.cbs.ctx,
                                                &sma_instance.active->props))) {
                     free_buffer(buf);
                     free_pool(sma_instance.active);
                     sma_instance.active = NULL;
                     buf = NULL;
                  }
               } else {
                  /* Emit 'expand pool' event */
                  if (sma_instance.cbs.expand_pool != NULL) {
                     sma_instance.cbs.expand_pool(sma_instance.cbs.ctx,
                                                  &sma_instance.active->props);
                  }
               }
            }
         }
      }
   }

   if (buf != NULL) {
      buf->props.group = grp;

      /* Update used pool size counter */
      sma_instance.active_used += grp_sz;

      /* Emit 'new buffer' event */
      if (sma_instance.cbs.new_buffer != NULL) {
         if (sma_instance.cbs.new_buffer(sma_instance.cbs.ctx, &buf->props)) {
            buf = NULL;
         }
      }
   }

   return buf;
}


void free_to_pool(struct sma_buffer *buf)
{
   struct sma_pool *buf_pool = buf->props.pool;

   /* Emit 'free buffer' event */
   if (sma_instance.cbs.free_buffer != NULL) {
      sma_instance.cbs.free_buffer(sma_instance.cbs.ctx, &buf->props);
   }

   /* Buffer is no longer grouped */
   buf->props.group = BUFFER_UNGROUPED;

   /* Update used pool size counter */
   if (sma_instance.active == buf_pool) {
      sma_instance.active_used -= calc_buffer_size(buf);
   }

   /* Coalesce with free buffers beside this one */
   if ((buf->next_in_pool != NULL) &&
       (buf->next_in_pool->props.group == BUFFER_UNGROUPED)) {
      free_buffer(buf->next_in_pool);
   }
   if ((buf->prev_in_pool != NULL) &&
       (buf->prev_in_pool->props.group == BUFFER_UNGROUPED)) {
      buf = buf->prev_in_pool;
      free_buffer(buf->next_in_pool);
   }

   /* Free buffer (and pool), if last buffer in non-active pool */
   if ((sma_instance.active != buf_pool) &&
       (buf_pool->allocd == buf) &&
       (buf->next_in_pool == NULL)) {
      free_buffer(buf);

      /* Emit 'free pool' event */
      if (sma_instance.cbs.free_pool != NULL) {
         sma_instance.cbs.free_pool(sma_instance.cbs.ctx, &buf_pool->props);
      }

      free_pool(buf_pool);
   }
}


struct sma_pool *alloc_pool(void)
{
   struct sma_pool *pool;
   char name[] = ("/" SMA_FD_NAME "-XXXXX");
   unsigned char attempts = 0;
   bool opened = false;

   pool = malloc(sizeof(struct sma_pool));
   if (pool != NULL) {
      do {
         /* A randomized pool name should help reduce collisions */
         sprintf(name + sizeof(SMA_FD_NAME) + 1, "%05X", rand() & 0xFFFF);
         pool->props.fd = shm_open(name,
                                   O_RDWR | O_CREAT | O_EXCL,
                                   S_IRUSR | S_IWUSR);
         if (pool->props.fd >= 0) {
            shm_unlink(name);
            pool->props.size = 0;
            pool->allocd = NULL;
            pool->map = NULL;
            pool->map_size = 0;
            pool->map_outdated = false;
            opened = true;
            break;
         } else {
            if (errno != EEXIST) {
               break;
            }
            attempts++;
         }
      } while (attempts < MAX_NAME_ATTEMPTS);

      if (!opened) {
         free(pool);
         pool = NULL;
      }
   }

   return pool;
}


void free_pool(struct sma_pool *pool)
{
   if (pool->map != NULL) {
      munmap(pool->map, pool->map_size);
   }

   close(pool->props.fd);
   free(pool);
}


struct sma_buffer *alloc_buffer(struct sma_buffer *last, size_t offset)
{
   struct sma_buffer *buf;
   struct sma_buffer_properties props = {
      NULL,
      BUFFER_UNGROUPED,
      sma_instance.active,
      offset
   };

   /* Allocate and intialize a new buffer (including linking in to pool) */
   buf = malloc(sizeof(struct sma_buffer));
   if (buf != NULL) {
      memcpy(&buf->props, &props, sizeof(struct sma_buffer_properties));
      buf->group_resized = false;
      buf->prev = NULL;
      buf->next = NULL;

      if (last == NULL) {
         buf->prev_in_pool = NULL;
         buf->next_in_pool = NULL;
         sma_instance.active->allocd = buf;
      } else {
         buf->prev_in_pool = last;
         buf->next_in_pool = last->next_in_pool;
         last->next_in_pool = buf;
         if (buf->next_in_pool != NULL) {
            buf->next_in_pool->prev_in_pool = buf;
         }
      }
   }

   return buf;
}


void free_buffer(struct sma_buffer *buf)
{
   struct sma_pool *buf_pool = buf->props.pool;

   /* Remove from pool */
   if (buf->prev_in_pool != NULL) {
      buf->prev_in_pool->next_in_pool = buf->next_in_pool;
   } else {
      buf_pool->allocd = buf->next_in_pool;
   }
   if (buf->next_in_pool != NULL) {
      buf->next_in_pool->prev_in_pool = buf->prev_in_pool;
   }

   free(buf);
}
