#ifndef ACGL_INPUTHANDLER_H
#define ACGL_INPUTHANDLER_H

#include <SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

typedef int (*ACGL_ih_callback_t)(SDL_Event, void*);

typedef struct {
  SDL_Scancode* keycodes;
  size_t keycodes_size;
} ACGL_ih_keybinds_t;

typedef struct ACGL_ih_callback_node_t ACGL_ih_callback_node_t;

struct ACGL_ih_callback_node_t {
  ACGL_ih_callback_t callback;
  void* data;
  ACGL_ih_callback_node_t* next;
};

typedef struct {
  ACGL_ih_callback_node_t** keyCallbacks;
  size_t keyCallbacks_size;
  ACGL_ih_callback_node_t* windowCallbacks;
} ACGL_ih_eventdata_t;

extern ACGL_ih_keybinds_t* ACGL_ih_init_keybinds(const SDL_Scancode keycodes[], const size_t keycodes_size);
extern ACGL_ih_eventdata_t* ACGL_ih_init_eventdata(const size_t keycodes_size);
extern void ACGL_ih_deinit_keybinds(ACGL_ih_keybinds_t* keybinds);
extern void ACGL_ih_deinit_eventdata(ACGL_ih_eventdata_t* medata);

// inserts new event at head of ACGL_ih_callback_node_t linked list
extern void ACGL_ih_register_keyevent(ACGL_ih_eventdata_t* medata, Uint16 keytype, ACGL_ih_callback_t callback, void* data);
extern void ACGL_ih_register_windowevent(ACGL_ih_eventdata_t* medata, ACGL_ih_callback_t callback, void* data);

// check to see if there is a function registered with that event, and if so, deletes it
extern void ACGL_ih_deregister_keyevent(ACGL_ih_eventdata_t* medata, Uint16 keytype, ACGL_ih_callback_t callback);
extern void ACGL_ih_deregister_windowevent(ACGL_ih_eventdata_t* medata, ACGL_ih_callback_t callback);

extern bool ACGL_ih_handle_keyevent(SDL_Event event, ACGL_ih_keybinds_t* keybinds, ACGL_ih_eventdata_t* medata);
extern bool ACGL_ih_handle_windowevent(SDL_Event event, ACGL_ih_eventdata_t* medata);

#endif // ACGL_INPUTHANDLER_H
