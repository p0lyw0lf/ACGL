#ifndef ACGL_THREADS_H
#define ACGL_THREADS_H

#include <SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "common.h"

// How long of a gap we should have before enforcing min_tick
extern Uint32 ACGL_THREAD_DELAY_CUTOFF; // = 5

// Tick function to be called every loop iteration in a thread
// Returns false when loop should stop
typedef bool (*ACGL_tick_callback_t)(void*);

typedef struct ACGL_thread_data ACGL_thread_data_t;
struct ACGL_thread_data {
  SDL_mutex* mutex;
  bool running;
  Uint32 min_tick;
  void* extra_data; // passed to the wrapped tick function
};

typedef struct ACGL_thread ACGL_thread_t;
struct ACGL_thread {
  SDL_Thread* thread;
  ACGL_tick_callback_t setupfn;
  ACGL_tick_callback_t tickfn;
  ACGL_tick_callback_t cleanupfn;
  ACGL_destroy_callback_t extra_data_destroy;
  ACGL_thread_data_t* data;
};

// Creates a new thread to run, but does not start it
extern ACGL_thread_t* ACGL_thread_create(
  ACGL_tick_callback_t setupfn,
  ACGL_tick_callback_t tickfn,
  ACGL_tick_callback_t cleanupfn,
  Uint32 min_tick,
  void* extra_data,
  ACGL_destroy_callback_t extra_data_destroy
);
// Starts running a thread if it isn't running already. Returns nonzero when thread could not be started
extern int ACGL_thread_start(ACGL_thread_t* target, const char* thread_name);
// Stops a running thread. Returns same as return code of thread
extern int ACGL_thread_stop(ACGL_thread_t* target);
// Destroys a thread object, freeing all memory associated with it
extern void ACGL_thread_destroy(ACGL_thread_t* target);
// Function that actually runs the loop
// Returns 0 if tick function stops on its own
extern int ACGL_thread_mainloop(void* target);

#endif // ACGL_THREADS_H
