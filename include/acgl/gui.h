#ifndef ACGL_GUI_H
#define ACGL_GUI_H

#include <SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

typedef float ACGL_gui_pos_t;

// You can add these together to get more anchor points.
// Only points that make sense logically will be considered. For example,
// GUI_ANCHOR_RIGHT + GUI_ANCHOR_LEFT makes no sense, but
// GUI_ANCHOR_TOP + GUI_ANCHOR_LEFT does
enum ACGL_GUI_ANCHOR_POINTS {
  ACGL_GUI_ANCHOR_CENTER = 0b0000,
  ACGL_GUI_ANCHOR_TOP    = 0b0001,
  ACGL_GUI_ANCHOR_LEFT   = 0b0010,
  ACGL_GUI_ANCHOR_BOTTOM = 0b0100,
  ACGL_GUI_ANCHOR_RIGHT  = 0b1000
};

enum ACGL_GUI_NODE_TYPE {
  ACGL_GUI_NODE_FIXED_SIZE      = 0b000,
  ACGL_GUI_NODE_FILL_H          = 0b001,
  ACGL_GUI_NODE_FILL_W          = 0b010,
  ACGL_GUI_NODE_PRESERVE_ASPECT = 0b100,
  // just so we can "know" what properties are assigned
  ACGL_GUI_NODE_NO_FILL_H       = 0b000,
  ACGL_GUI_NODE_NO_FILL_W       = 0b000,
  ACGL_GUI_NODE_NO_PRESERVE_ASPECT = 0b000,
};

static const ACGL_gui_pos_t ACGL_GUI_DIM_NONE = -1;
static const ACGL_gui_pos_t ACGL_GUI_DIM_FILL = -2;

typedef struct ACGL_gui_object_t ACGL_gui_object_t;
typedef bool (*ACGL_gui_callback_t)(SDL_Renderer*, SDL_Rect, void*);

struct ACGL_gui_object_t {
  SDL_Renderer* renderer;
  SDL_mutex* mutex;
  ACGL_gui_callback_t callback; // is called before any of the childrens'
  void* callback_data;
  bool needs_update; // set this flag (after locking the mutex) whenever you
                     // want the node and all its children to redraw themselves.
                     // if set on a child, DOES NOT update the parent.

  // change the following data points to change the node's drawing behavior
  int anchor;
  int node_type;
  ACGL_gui_pos_t x, y;
  ACGL_gui_pos_t w, h;
  ACGL_gui_pos_t min_w, min_h;
  ACGL_gui_pos_t max_w, max_h;
  bool x_frac, y_frac;
  bool w_frac, h_frac;

  // allows you to construct a tree of ACGL_gui rendering objects
  // for safety, DO NOT EDIT THESE BY HAND, instead use one of the provided functions
  ACGL_gui_object_t* parent;
  ACGL_gui_object_t* prev_sibling;
  ACGL_gui_object_t* next_sibling;
  ACGL_gui_object_t* first_child;
  ACGL_gui_object_t* last_child;
};

typedef struct {
  SDL_Renderer* renderer;
  ACGL_gui_object_t* root;
} ACGL_gui_t;


// Serves as an entrypoint to the render tree. Traverses if DFS-style.
// The tree expects the background elements to be at the front of the linkedlist
extern bool ACGL_gui_render(ACGL_gui_t* ACGL_gui); // returns: did render
extern ACGL_gui_t* ACGL_gui_init(SDL_Renderer* renderer); // creates a new ACGL_gui_t
extern void ACGL_gui_destroy(ACGL_gui_t* ACGL_gui); // destroys the ACGL_gui_t and the entire subtree

extern ACGL_gui_object_t* ACGL_gui_node_init(SDL_Renderer* renderer, ACGL_gui_callback_t callback, void* data);
extern bool ACGL_gui_node_render(ACGL_gui_object_t* node, SDL_Rect location); // returns: did render
extern void ACGL_gui_node_add_child_front(ACGL_gui_object_t* parent, ACGL_gui_object_t* child);
extern void ACGL_gui_node_add_child_back(ACGL_gui_object_t* parent, ACGL_gui_object_t* child);
extern void ACGL_gui_node_remove_child(ACGL_gui_object_t* parent, ACGL_gui_object_t* child);
extern void ACGL_gui_node_remove_all_children(ACGL_gui_object_t* parent);
// removing a single child does not automatically destroy it, removing all children does
extern void ACGL_gui_node_destroy(ACGL_gui_object_t* node);

extern bool ACGL_gui_blank_callback(SDL_Renderer* renderer, SDL_Rect location, void* data);
extern bool ACGL_gui_force_update(ACGL_gui_t* gui);
#endif //ACGL_GUI_H
