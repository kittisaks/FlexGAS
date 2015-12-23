#define _SCHED
#include "rec_internal.h"
#include "sched_internal.h"

#define COMP_NAME "Sched"

int find_exec_res(exec_resource ** e_res, int * ret_num, 
    sched_group group, int req_num) {

    if (!resource_list_available)
        return -1;

    exec_resource * e_res_tmp = new exec_resource[req_num];
    int res_num = 0;

    pthread_mutex_lock(&resource_list_mtx); {
        for (int idx=0;idx<resource_count;idx++) {

            if (res_num == req_num)
                break;

            resource * res = &resource_list[idx]; 
            pthread_mutex_lock(&(res->lock)); {
                if (res->num_occupants > 0) {
                    bool deny = false;
                    for (int jIdx=0;jIdx<res->num_occupants;jIdx++) {
                        if (res->rec_levels[jIdx] == group->rec_level) {
                            deny = true;
                            break;
                        }
                    }

                    if (deny) {
                        pthread_mutex_unlock(&(res->lock));
                        continue;
                    }
                }

                sched_device device_tmp;
                int ret = schedCreateDevice(res->hostname, sched_link_inet,
                    ivm_cpu, 0, group, &device_tmp);
                if (ret != 0) {
                    pthread_mutex_unlock(&(res->lock));
                    continue;
                }

                e_res_tmp[res_num].res    = res;
                e_res_tmp[res_num].device = device_tmp;

                res->rec_levels.push_back(group->rec_level);
                res->num_occupants++;
                res_num++;
            }
            pthread_mutex_unlock(&(res->lock));
        }
    }
    pthread_mutex_unlock(&resource_list_mtx);

    *e_res   = e_res_tmp;
    *ret_num = res_num;

    return 0;
}

void release_exec_res(
    exec_resource * e_res, int num_e_res, sched_group group) {

    pthread_mutex_lock(&resource_list_mtx);
    for (int idx=0;idx<num_e_res;idx++) {
        resource * res = e_res[idx].res;
        pthread_mutex_lock(&(res->lock)); {
            for (int jIdx=0;jIdx<res->num_occupants;jIdx++) {
                if (res->rec_levels[jIdx] == group->rec_level) {
                    res->rec_levels.erase(
                        res->rec_levels.begin() + jIdx);
                    break;
                }
            }
            res->num_occupants--;
        }
        pthread_mutex_unlock(&(res->lock));
        schedDestroyDevice(e_res[idx].device);
    }
    pthread_mutex_unlock(&resource_list_mtx);
    delete [] e_res;
}

////////////////////////////////// JOB SCHED //////////////////////////////////

int init_job_handler(job_context * job_ctx, sched_event * event) {

    CHECK_ZF(   schedCreateSchedHandle(&(job_ctx->event_handle)),
        "Error at line %d", __LINE__);
    CHECK_ZF(   schedRegisterHandle(job_ctx->event_handle, SCHED_INST_JOB,
        event->jid, 0, JOB_EVENT_START_COMP | JOB_EVENT_EXEC_COMPLETE),
        "Error at line %d", __LINE__);

    return 0;
}

void * job_sched_routine(void * arg) {

    th_event    * th_ev = (th_event *) arg;
    thread_t    * self  = th_ev->th;

    job_context   job_ctx;
    job_ctx.sys_event   = th_ev->event;
    init_job_handler(&job_ctx, job_ctx.sys_event);

    AtomicVector<sched_event>               q_job_event;
    AtomicMap<sched_event *, sched_event *> r_job_event;

    TRACE("[Th: %p]: [Job %d]: Monitored.", self, job_ctx.sys_event->jid);

    do {

        //Retrieve and queue events
        sched_event event;
        int ret = schedPollEvent(job_ctx.event_handle, &event);
        if ((ret == -1) && (q_job_event.size() == 0)) {
            schedWaitEvent(job_ctx.event_handle, &event);
            q_job_event.push_back(event);
        }
        else if (ret == 0) {
            q_job_event.push_back(event);
        }

        //Process events
        if (q_job_event.size() == 0)
            continue;
        q_job_event.fetch(&event, 0);

        //Allocate resources for the computation launch
        exec_resource * e_res;
        int             ret_num, req_num = DEF_RES_CNT;
        find_exec_res(&e_res, &ret_num, event.group, req_num);
//        TRACE("ATTEMPT: %s %d %d",
//            event.comp->comp_id, 
//            event.comp->config.max_pes_num,
//            event.group->rec_level);
        if (ret_num == 0) {
            q_job_event.push_back(event);
            usleep(10000);
            continue;
        }

        th_event * th_ev = new th_event();
        th_ev->th        = new thread_t();
        th_ev->event     = new sched_event();
        *(th_ev->event)  = event;

        group_context * group_ctx = new group_context();
        group_ctx->job_event      = new sched_event();
        group_ctx->th_group       = new thread_t();
        *(group_ctx->job_event)   = event;
        group_ctx->e_res          = e_res;
        group_ctx->num_e_res      = ret_num;
        thread_create(group_ctx->th_group, group_sched_routine, 
            (void *) group_ctx);

    } while(1);

    pthread_exit(NULL);
}

///////////////////////////////// GROUP SCHED /////////////////////////////////

int init_group_handler(group_context * group_ctx, sched_event * event) {

    CHECK_ZF(   schedCreateSchedHandle(&(group_ctx->event_handle)),
        "Error at line %d", __LINE__);
    CHECK_ZF(   schedRegisterHandle(group_ctx->event_handle, SCHED_INST_GROUP,
        event->jid, event->gid, GROUP_EVENT_PE_EXIT),
        "Error at line %d", __LINE__);

    return 0;
}

void * group_sched_routine(void * arg) {

    group_context * group_ctx = (group_context *) arg;
    init_group_handler(group_ctx, group_ctx->job_event);

    TRACE("[Th: %p]: [Job %d]: [Group %d]:  Scheduled.", group_ctx->th_group,
        group_ctx->job_event->jid, group_ctx->job_event->gid);
    TRACE("\n\tComputation: %s\n\t# of PEs: %d (%d)",
        group_ctx->job_event->comp->comp_id, 
        group_ctx->job_event->comp->config.max_pes_num,
        group_ctx->job_event->group->rec_level);

    uint32_t max_pes_num = group_ctx->job_event->comp->config.max_pes_num;
    uint32_t spawned_pes = 0;
    int      active_pes, idx, ret;
    do {
        active_pes = 0;
        for (idx=0;idx<group_ctx->num_e_res;idx++) {
            if (spawned_pes < max_pes_num) {
                ret = schedSpawnPes(group_ctx->e_res[idx].device,
                    group_ctx->job_event->comp, 1, NULL);
                if ((ret == 0) &&  (spawned_pes < max_pes_num)) { 
                    spawned_pes++;
                    active_pes++;
                }
            }
        }

        for (idx=0;idx<active_pes;idx++) {
            sched_event event;
            schedWaitEvent(group_ctx->event_handle, &event);
        }
    } while (spawned_pes < max_pes_num);

    schedAckEvent(*group_ctx->job_event);
    release_exec_res(
        group_ctx->e_res, group_ctx->num_e_res, group_ctx->job_event->group);

    pthread_exit(NULL);
}

