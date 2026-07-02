#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <dbChannel.h>
#include <dbAccess.h>
#include <dbEvent.h>
#include <dbCommon.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsMutex.h>
#include <epicsTime.h>
#include <iocsh.h>
#include <epicsExport.h>

#define MAX_ELEMS 3333

typedef struct {
    double values[MAX_ELEMS];
    long n;
    epicsTimeStamp timestamp;
    unsigned long sequence;
} WorkerArgs;

typedef struct {
    double values[MAX_ELEMS];
    long n;
    epicsTimeStamp timestamp;
    unsigned long sequence;
    unsigned long pending;
} SharedBuffer;

static SharedBuffer shared;
static epicsMutexId sharedMutex = NULL;
static epicsEventId dataEvent = NULL;
static int dispatcherStarted = 0;

static int isPrimeLong(long long x)
{
    long long i;

    if (x < 2) return 0;
    if (x == 2) return 1;
    if (x % 2 == 0) return 0;

    for (i = 3; i <= x / i; i += 2) {
        if (x % i == 0) return 0;
    }

    return 1;
}

static void workerThread(void *arg)
{
    WorkerArgs *w = (WorkerArgs *)arg;
    double sum = 0.0;
    long i;
    long long isum;
    char timeText[64];

    for (i = 0; i < w->n; i++) {
        sum += w->values[i];
    }

    isum = (long long)llround(sum);

    epicsTimeToStrftime(timeText, sizeof(timeText),
                        "%Y-%m-%dT%H:%M:%S.%06fZ",
                        &w->timestamp);

    printf("New worker threadId : %p\n", (void*)epicsThreadGetIdSelf());
    printf("Worker #%lu: elements=%ld sum=%lld\n",
           w->sequence, w->n, isum);

    if (isPrimeLong(isum)) {
        printf("worker : data is prime : %lld at %s\n", isum, timeText);
    }

    free(w);
}

static void dispatcherThread(void *arg)
{
    (void)arg;

    while (1) {
        epicsEventWait(dataEvent);

        while (1) {
            WorkerArgs *w;

            epicsMutexMustLock(sharedMutex);

            if (shared.pending == 0) {
                epicsMutexUnlock(sharedMutex);
                break;
            }

            w = (WorkerArgs *)calloc(1, sizeof(WorkerArgs));
            if (!w) {
                shared.pending--;
                epicsMutexUnlock(sharedMutex);
                printf("threadExample: calloc failed\n");
                continue;
            }

            /* Synchronisation: protected copy from SharedBuffer to private WorkerArgs. */
            memcpy(w->values, shared.values, sizeof(double) * shared.n);
            w->n = shared.n;
            w->timestamp = shared.timestamp;
            w->sequence = shared.sequence;
            shared.pending--;

            epicsMutexUnlock(sharedMutex);

            epicsThreadCreate("threadExampleWorker",
                              epicsThreadPriorityLow,
                              epicsThreadGetStackSize(epicsThreadStackSmall),
                              workerThread,
                              w);
        }
    }
}

static void eventCallback(void *user_arg, dbChannel *chan,
                          int eventsRemaining, db_field_log *pfl)
{
    double local[MAX_ELEMS];
    long nRequest;
    long options = 0;
    dbCommon *prec;
    long status;

    (void)user_arg;
    (void)eventsRemaining;
    (void)pfl;

    nRequest = dbChannelFinalElements(chan);
    if (nRequest > MAX_ELEMS) {
        nRequest = MAX_ELEMS;
    }

    status = dbChannelGet(chan, DBR_DOUBLE, local, &options, &nRequest, NULL);
    if (status) {
        printf("threadExample: dbChannelGet failed\n");
        return;
    }

    prec = (dbCommon *)dbChannelRecord(chan);

    /* Synchronisation: short non-blocking callback, only copy + signal. */
    epicsMutexMustLock(sharedMutex);

    memcpy(shared.values, local, sizeof(double) * nRequest);
    shared.n = nRequest;
    shared.timestamp = prec->time;
    shared.sequence++;
    shared.pending++;

    epicsMutexUnlock(sharedMutex);

    epicsEventSignal(dataEvent);
}

static void threadExample(const char *pvName)
{
    dbChannel *chan;
    dbEventCtx ctx;
    dbEventSubscription sub;
    long elements;

    if (!pvName || strlen(pvName) == 0) {
        printf("Usage: threadExample pv-name\n");
        printf("Example: threadExample ahdo1293:compressExample\n");
        return;
    }

    if (!sharedMutex) {
        sharedMutex = epicsMutexMustCreate();
    }

    if (!dataEvent) {
        dataEvent = epicsEventCreate(epicsEventEmpty);
    }

    if (!dispatcherStarted) {
        dispatcherStarted = 1;
        epicsThreadCreate("threadExampleDispatcher",
                          epicsThreadPriorityMedium,
                          epicsThreadGetStackSize(epicsThreadStackMedium),
                          dispatcherThread,
                          NULL);
    }

    chan = dbChannelCreate(pvName);
    if (!chan) {
        printf("threadExample: invalid PV name: %s\n", pvName);
        return;
    }

    if (dbChannelOpen(chan)) {
        printf("threadExample: dbChannelOpen failed for PV: %s\n", pvName);
        dbChannelDelete(chan);
        return;
    }

    elements = dbChannelFinalElements(chan);

    printf("threadExample: '%s' NSAM=%ld\n", pvName, elements);

    ctx = db_init_events();
    if (!ctx) {
        printf("threadExample: db_init_events failed\n");
        return;
    }

    sub = db_add_event(ctx, chan, eventCallback, NULL, DBE_VALUE);
    if (!sub) {
        printf("threadExample: db_add_event failed\n");
        return;
    }

    db_start_events(ctx, "threadExample", NULL, NULL, 0);
    db_event_enable(sub);
    db_post_single_event(sub);

    printf("threadExample: subscription active for '%s'\n", pvName);
}

static const iocshArg threadExampleArg0 = {"pv-name", iocshArgString};
static const iocshArg *threadExampleArgs[] = {&threadExampleArg0};

static const iocshFuncDef threadExampleFuncDef = {
    "threadExample",
    1,
    threadExampleArgs
};

static void threadExampleCallFunc(const iocshArgBuf *args)
{
    threadExample(args[0].sval);
}

void threadExampleRegister(void)
{
    iocshRegister(&threadExampleFuncDef, threadExampleCallFunc);
}

epicsExportRegistrar(threadExampleRegister);
