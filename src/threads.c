#include "threads.h"
#include "contracts.h"

Uint32 ACGL_THREAD_DELAY_CUTOFF = 5;

// Safety functions
bool __acgl_is_thread_data(ACGL_thread_data_t* data) {
  if (data == NULL) {
    return false;
  }
  if (data->mutex == NULL) {
    return false;
  }
  if (data->min_tick < ACGL_THREAD_DELAY_CUTOFF) {
    return false;
  }
  return true;
}

bool __acgl_is_thread(ACGL_thread_t* target) {
  if (target == NULL) {
    return false;
  }
  if (target->data == NULL) {
    return false;
  }
  if (target->extra_data_destroy == NULL && target->data->extra_data != NULL) {
    return false;
  }
  if (target->data->running && target->thread == NULL) {
    return false;
  }
  if (!target->data->running && target->thread != NULL) {
    return false;
  }

  return __acgl_is_thread_data(target->data);
}


ACGL_thread_t* ACGL_thread_create(ACGL_tick_callback_t setupfn, ACGL_tick_callback_t tickfn, ACGL_tick_callback_t cleanupfn, Uint32 min_tick, void* extra_data, ACGL_destroy_callback_t extra_data_destroy) {
  ACGL_thread_data_t* data = (ACGL_thread_data_t*)malloc(sizeof(ACGL_thread_data_t));
  if (data == NULL) {
    fprintf(stderr, "Error: could not malloc thread data in ACGL_thread_create!\n");
    return NULL;
  }
  data->mutex = SDL_CreateMutex();
  data->running = false;
  data->min_tick = min_tick;
  data->extra_data = extra_data;

  ACGL_thread_t* thread = (ACGL_thread_t*)malloc(sizeof(ACGL_thread_t));
  if (thread == NULL) {
    fprintf(stderr, "Error: could not malloc thread object in ACGL_thread_create!\n");
    SDL_DestroyMutex(data->mutex);
    free(data);
    return NULL;
  }
  thread->thread = NULL;
  thread->setupfn = setupfn;
  thread->tickfn = tickfn;
  thread->cleanupfn = cleanupfn;
  thread->extra_data_destroy = extra_data_destroy;
  thread->data = data;

  ENSURES(__acgl_is_thread(thread));
  return thread;
}

int ACGL_thread_start(ACGL_thread_t* target, const char* thread_name) {
  REQUIRES(__agcl_is_thread(target));

  if (target == NULL) {
    fprintf(stderr, "Error: Attempted to start NULL thread!\n");
    return -1;
  }

  if (SDL_LockMutex(target->data->mutex) != 0) {
    fprintf(stderr, "Error locking mutex in ACGL_thread_start. SDL_Error: %s", SDL_GetError());
    return -1;
  }

  if (target->data->running) {
    fprintf(stderr, "Error: Attempted to start already-running thread!\n");
    SDL_UnlockMutex(target->data->mutex);
    return -1;
  }

  target->data->running = true;
  target->thread = SDL_CreateThread(
    &ACGL_thread_mainloop,
    thread_name,
    target
  );

  SDL_UnlockMutex(target->data->mutex);

  ENSURES(__agcl_is_thread(target));
  return 0;
}

int ACGL_thread_stop(ACGL_thread_t* target) {

  REQUIRES(__acgl_is_thread(target));
  if (target == NULL) {
    fprintf(stderr, "Error: tried to stop a NULL thread in ACGL_thread_stop\n");
    return false;
  }

  if (SDL_LockMutex(target->data->mutex) != 0) {
    fprintf(stderr, "Error locking mutex in ACGL_thread_stop. SDL_Error: %s", SDL_GetError());
    return false;
  }

  if (!target->data->running) {
    fprintf(stderr, "Error: thread to stop already-stopped thread in ACGL_thread_stop\n");
    SDL_UnlockMutex(target->data->mutex);
    return false;
  }

  target->data->running = false;
  SDL_UnlockMutex(target->data->mutex);

  int result;
  SDL_WaitThread(target->thread, &result);

  ENSURES(__acgl_is_thread(target));
  return result;
}

void ACGL_thread_destroy(ACGL_thread_t* target) {
  // This is a very unsafe destruction, doesn't check to stop running first
  if (target != NULL) {
    if (target->data != NULL) {
      if (target->data->extra_data != NULL) {
        if (target->extra_data_destroy != NULL) {
          (*target->extra_data_destroy)(target->data->extra_data);
        }
        target->data->extra_data = NULL;
      }
      if (target->data->mutex != NULL) {
        SDL_DestroyMutex(target->data->mutex);
        target->data->mutex = NULL;
      }
      free(target->data);
      target->data = NULL;
    }
    free(target);
  }
}

int ACGL_thread_mainloop(void* data) {
  ACGL_thread_t* target = (ACGL_thread_t*)data;
  REQUIRES(__acgl_is_thread(target));
  // Each thread needs to re-seed independently
  // for whatever reason
  // I can't believe they didn't make `rand` return
  // well-seeded numbers by default
  srand((unsigned)time(NULL));

  bool unlocked_running = true;
  if (target->setupfn != NULL) {
    if (SDL_LockMutex(target->data->mutex) != 0) {
      fprintf(stderr, "Could not lock mutex while setting up in ACGL_thread_mainloop! SDL_Error: %s", SDL_GetError());
      return 1;
    }
    unlocked_running = (*target->setupfn)(target->data->extra_data);
    SDL_UnlockMutex(target->data->mutex);
  }

  while (unlocked_running) {
    if (SDL_LockMutex(target->data->mutex) != 0) {
      fprintf(stderr, "Could not lock mutex in loop in ACGL_thread_mainloop! SDL_Error: %s", SDL_GetError());
      return 1;
    }

    if (!target->data->running) {
      // quit loop immediately
      unlocked_running = false;
      SDL_UnlockMutex(target->data->mutex);
      break;
    }

    Uint32 started = SDL_GetTicks();
    if (target->tickfn != NULL) {
      unlocked_running = (*target->tickfn)(target->data->extra_data);
    }
    // We need to unlock mutex as soon as possible so we can be
    // alerted of stopping
    SDL_UnlockMutex(target->data->mutex);

    Uint32 elapsed = SDL_GetTicks() - started;
    if (
      target->data->min_tick > ACGL_THREAD_DELAY_CUTOFF && 
      elapsed < target->data->min_tick - ACGL_THREAD_DELAY_CUTOFF
    ) {
      SDL_Delay(target->data->min_tick - elapsed);
    }
  }

  if (target->cleanupfn != NULL) {
    if (SDL_LockMutex(target->data->mutex) != 0) {
      fprintf(stderr, "Could not lock mutex while cleaning up in ACGL_thread_mainloop! SDL_Error: %s", SDL_GetError());
      return 1;
    }
    unlocked_running = (*target->cleanupfn)(target->data->extra_data);
    SDL_UnlockMutex(target->data->mutex);
  }

  return 0;
}
