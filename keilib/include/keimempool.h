#ifndef KMEMPOOL_H_
#define KMEMPOOL_H_

#include <list>
#include <algorithm>
#include <mutex>

extern pid_t gettid();

#define KMEMPOOL_D 1
#define KMEMPOOL_ERROR printf
#define KMEMPOOL_WARN printf
#define KMEMPOOL_INFO printf
#define KMEMPOOL_DEBUG(fmt, arg...) \
    do                              \
    {                               \
        if (KMEMPOOL_D)             \
        {                           \
            printf(fmt, ##arg);     \
        }                           \
    } while (0)

#ifdef __x86_64__
typedef uint64_t tulong;
typedef int64_t tlong;
#else
typedef uint32_t tulong;
typedef int32_t tlong;
#endif
class memunit
{
public:
    enum memunit_e
    {
        FREE,
        USED
    };

    memunit(tulong addr = 0, size_t size = 0, uint32_t aligment = 4, uint32_t flag = FREE, pid_t rtid = 0)
        : addr(reinterpret_cast<tulong>(addr)),
          size(size),
          aligment(aligment),
          flag(flag),
          request_tid(rtid){

          };
    tulong addr;
    size_t size;
    uint32_t aligment;
    uint32_t flag;
    pid_t request_tid;
};

class keimempool
{
public:
    keimempool(void *addr, size_t size, uint32_t flag = 0) : def_ali(8)
    {
        bottom = reinterpret_cast<tulong>(addr);
        top = bottom + size;
        free_mulist.push_back(memunit(bottom, size));
        KMEMPOOL_INFO("keimempool 0x%lx-%lx\n", bottom, top);
    };
    ~keimempool(){};

    void *memrequest(size_t size)
    {
        tulong result = 0;
        std::lock_guard<std::mutex> guard(mlock);

        trace_p();
        if (!size)
        {
            return NULL;
        }
        size = round_up(size, def_ali);
        auto i = free_mulist.begin();
        i = free_mulist.begin();
        for (; i != free_mulist.end(); i++)
        {
            tulong req_start = round_up(i->addr, def_ali);
            tlong rest_sz = (i->addr + i->size) - (req_start + size);
            if (rest_sz >= 0)
            {
                tlong ali_sz = (req_start + size) - i->addr;
                result = req_start;
                if (rest_sz > 0)
                {
                    memunit new_free_mu((req_start + size), rest_sz, def_ali, memunit::FREE);
                    auto ir = i;
                    ir++;
                    free_mulist.insert(ir, new_free_mu);
                }
                if(req_start > i->addr)
                {
                    memunit new_free_mu(i->addr, req_start - i->addr, def_ali, memunit::FREE);
                    free_mulist.insert(i, new_free_mu);
                }

                memunit new_used_mu(req_start, size, def_ali, memunit::USED, gettid());
                ins_used_mu(new_used_mu);

                free_mulist.erase(i);
                break;
            }
        }

        if (result)
        {
            if ((result < bottom) || ((result + size) > top))
            {
                result = 0;
                KMEMPOOL_DEBUG("%d memrequest error \n", gettid());
                abort();
            }
        }
        KMEMPOOL_INFO("%d buf 0x%lx-0x%lx\n", gettid(),
                      result, result + size);
        return reinterpret_cast<void *>(result);
    };
    int memdelete(void *_addr)
    {
        tulong addr = reinterpret_cast<tulong>(_addr);
        std::lock_guard<std::mutex> guard(mlock);
        size_t free_sz = 0;
        int ret = 0;
        auto i = used_mulist.begin();
        for (; i != used_mulist.end(); i++)
        {
            if ((addr == i->addr))
            {
                memunit new_free_mu = *i;
                new_free_mu.flag = memunit::FREE;
                free_sz = i->size;
                ins_free_mu(new_free_mu);
                used_mulist.erase(i);
                break;
            }
        }
        if(!free_sz)
        {
            ret = -1;
        }
        KMEMPOOL_INFO("%d memdelete %s 0x %lx-0x %lx\n", gettid(), free_sz ? "done" : "error", addr, addr + free_sz);
        return 0;
    };
    void memclear(){};

    void inline trace_p() const
    {
        auto i = free_mulist.begin();
        auto e = free_mulist.end();
        for (; i != e; i++)
        {
            KMEMPOOL_DEBUG("%d traced free_l 0x %lx-0x %lx\n", gettid(), i->addr, i->addr + i->size);
        }
        i = used_mulist.begin();
        e = used_mulist.end();
        for (; i != e; i++)
        {
            KMEMPOOL_DEBUG("%d traced %d used_l 0x %lx-0x %lx\n", gettid(), i->request_tid, i->addr, i->addr + i->size);
        }
    }

private:
    uint32_t def_ali;
    std::mutex mlock;
    tulong bottom;
    tulong top;
    inline tulong round_up(tulong o, uint32_t ali)
    {
        return reinterpret_cast<tulong>(((uint64_t)(o) + (ali)-1) & -((ali)));
    };
    inline void ins_free_mu(memunit &mu)
    {
        auto b = free_mulist.begin();
        auto e = free_mulist.end();
        auto i = b;
        for (; i != e; i++)
        {
            if (i->addr > mu.addr)
            {
                break;
            }
        }
        if (i != e)
        {
            KMEMPOOL_DEBUG("%d insert before 0x %lx\n", gettid(), i->addr);
        }
        auto cur = free_mulist.insert(i, mu);
        compat_free_mu(cur);
    };
    inline void ins_used_mu(memunit &mu)
    {
        auto b = used_mulist.begin();
        auto e = used_mulist.end();
        auto i = b;
        for (; i != e; i++)
        {
            if (i->addr > mu.addr)
            {
                break;
            }
        }
        auto cur = free_mulist.insert(i, mu);
    };
    inline void compat_free_mu(std::list<memunit>::iterator &i)
    {
        auto b = free_mulist.begin();
        auto e = free_mulist.end();
        auto il = i;
        auto ir = i;
        (il == b) ? (il = e) : (il--);
        ir++;
        if (il != e)
        {
            KMEMPOOL_DEBUG("il 0x %lx-0x %lx\n", il->addr, il->addr + il->size);
            if ((il->addr + il->size) == i->addr)
            {
                i->addr = il->addr;
                i->size += il->size;
                KMEMPOOL_DEBUG("compat 0x %lx-0x %lx\n", il->addr, i->addr + i->size);
                free_mulist.erase(il);
            }
        }
        if (ir != e)
        {
            KMEMPOOL_DEBUG("ir 0x %lx-0x %lx\n", ir->addr, ir->addr + ir->size);
            if ((i->addr + i->size) == ir->addr)
            {
                i->size += ir->size;
                KMEMPOOL_DEBUG("compat 0x %lx-0x %lx\n", i->addr, ir->addr + ir->size);
                free_mulist.erase(ir);
            }
        }
    };
    std::list<memunit> free_mulist;
    std::list<memunit> used_mulist;
};

#endif // KMEMPOOL_H_