#include "gui.h"
#include "gui_safety.h"
#include "contracts.h"

#ifndef min
#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })
#endif
#ifndef max
#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })
#endif

bool ACGL_gui_force_update(ACGL_gui_t* gui) {
    REQUIRES(__ACGL_is_gui_t(gui));
   
    bool old_update = gui->root->needs_update;
    gui->root->needs_update = true;
    ENSURES(__ACGL_is_gui_t(gui));
    return !old_update;
}

bool ACGL_gui_render(ACGL_gui_t* gui) {
  REQUIRES(__ACGL_is_gui_t(gui));

  int w, h;
  // Initialize memory just in case
  w = 0;
  h = 0;
  SDL_GL_GetDrawableSize(gui->window, &w, &h);
  SDL_Rect location = {0, 0, w, h};

  bool output = ACGL_gui_node_render(gui, gui->root, location);
  ENSURES(__ACGL_is_gui_t(gui));
  return output;
}

ACGL_gui_t* ACGL_gui_init(SDL_Window* window) {
  assert(window != NULL);

  ACGL_gui_t* gui = (ACGL_gui_t*)malloc(sizeof(ACGL_gui_t));
  if (gui == NULL) {
    fprintf(stderr, "Error! Could not malloc gui in ACGL_gui_init\n");
    return NULL;
  }
  gui->window = window;

  gui->root = NULL;
  gui->root = ACGL_gui_node_init(gui, NULL, NULL, NULL);
  if (gui->root == NULL) {
    fprintf(stderr, "Error! could not create gui root node in ACGL_gui_init\n");
    free(gui);
    return NULL;
  }

  ENSURES(__ACGL_is_gui_t(gui));
  return gui;
}

void ACGL_gui_destroy(ACGL_gui_t* gui) {
  REQUIRES(__ACGL_is_gui_t(gui));

  if (gui->root != NULL) {
    ACGL_gui_node_destroy(gui->root);
  }
  gui->root = NULL;

  // don't destroy window, could just be switching away from ACGL
  gui->window = NULL;

  free(gui);
}

ACGL_gui_object_t* ACGL_gui_node_init(ACGL_gui_t* gui, ACGL_render_callback_t render, ACGL_destroy_callback_t destroy, void* data) {
  REQUIRES(__ACGL_is_gui_t(gui));

  ACGL_gui_object_t* node = (ACGL_gui_object_t*)malloc(sizeof(ACGL_gui_object_t));
  if (node == NULL) {
    fprintf(stderr, "Error! could not malloc node in ACGL_gui_node_init\n");
    return NULL;
  }

  node->mutex = SDL_CreateMutex();
  if (node->mutex == NULL) {
    fprintf(stderr, "Could not create mutex in ACGL_gui_node_init! SDL Error: %s\n", SDL_GetError());
    free(node);
    return NULL;
  }

  node->render_callback = render;
  node->destroy_callback = destroy;
  node->callback_data = data;
  node->needs_update = true;

  // these defaults make the node fill up all available space in its parent
  node->node_type = ACGL_GUI_NODE_FILL_H + ACGL_GUI_NODE_FILL_W + ACGL_GUI_NODE_NO_PRESERVE_ASPECT;
  node->anchor = ACGL_GUI_ANCHOR_CENTER;
  node->x = 0;
  node->y = 0;
  node->w = 1;
  node->h = 1;
  node->min_w = ACGL_GUI_DIM_NONE;
  node->min_h = ACGL_GUI_DIM_NONE;
  node->max_w = ACGL_GUI_DIM_NONE;
  node->max_h = ACGL_GUI_DIM_NONE;
  node->x_frac = false;
  node->y_frac = false;
  node->w_frac = false;
  node->h_frac = false;


  node->parent = NULL;
  node->prev_sibling = NULL;
  node->next_sibling = NULL;
  node->first_child = NULL;
  node->last_child = NULL;

  ENSURES(__ACGL_is_gui_object_t(node));
  return node;
}

bool ACGL_gui_node_render(ACGL_gui_t* gui, ACGL_gui_object_t* node, SDL_Rect location) {
  REQUIRES(__ACGL_is_gui_t(gui));
  REQUIRES(__ACGL_is_gui_object_t(node));
  
  if (SDL_LockMutex(node->mutex) != 0) {
    fprintf(stderr, "Could not lock mutex in ACGL_gui_node_render. SDL_Error: %s\n", SDL_GetError());
    return false;
  }

  bool return_val = false;

  // compute rectangle that we are supposed to draw in
  SDL_Rect sublocation = location;
  ACGL_gui_pos_t sblw, sblh;
  sblw = (ACGL_gui_pos_t)sublocation.w;
  sblh = (ACGL_gui_pos_t)sublocation.h;

  if (node->node_type & ACGL_GUI_NODE_PRESERVE_ASPECT) {
    ACGL_gui_pos_t minw, maxw, minh, maxh;

	  if (node->min_w != ACGL_GUI_DIM_NONE) { minw = node->min_w; }
	  else { minw = 0.0; }
	  if (node->max_w != ACGL_GUI_DIM_NONE) { maxw = min(node->max_w, (ACGL_gui_pos_t)location.w); }
	  else { maxw = (ACGL_gui_pos_t)location.w; }
	  if (node->min_h != ACGL_GUI_DIM_NONE) { minh = node->min_h; }
	  else { minh = 0.0; }
	  if (node->max_h != ACGL_GUI_DIM_NONE) { maxh = min(node->max_h, (ACGL_gui_pos_t)location.h); }
	  else { maxh = (ACGL_gui_pos_t)location.h; }
	
    if ( (node->node_type & ACGL_GUI_NODE_FILL_W) && !(node->node_type & ACGL_GUI_NODE_FILL_H) ) {
      // fill all the available space first
      sblw = (ACGL_gui_pos_t)location.w;

      // cap off minimum and maximum
      if (node->min_w != ACGL_GUI_DIM_NONE && sblw < minw) {
        sblw = minw;
      }
      if (node->max_w != ACGL_GUI_DIM_NONE && sblw > maxw) {
        sblw = maxw;
      }

      // then set height from that
      sblh = sblw * node->h / node->w;

      // then cap off minimum and maximum of other dimension
      bool capped = false;
      if (node->min_h != ACGL_GUI_DIM_NONE && sblh < minh) {
        sblh = minh;
        capped = true;
      }
      if (node->max_h != ACGL_GUI_DIM_NONE && sblh > maxh) {
        sblh = maxh;
        capped = true;
      }

      // if we did cap, we need to re-set the fill to something maybe non-filled
      if (capped) {
        sblw = sblh * node->w / node->h;
      }

    } else if ( !(node->node_type & ACGL_GUI_NODE_FILL_W) && (node->node_type & ACGL_GUI_NODE_FILL_H) ) {
	    // fill all the available space first
      sblh = (ACGL_gui_pos_t)location.h;

      // cap off minimum and maximum
      if (node->min_h != ACGL_GUI_DIM_NONE && sblh < minh) {
        sblh = minh;
      }
      if (node->max_h != ACGL_GUI_DIM_NONE && sblh > maxh) {
        sblh = maxh;
      }

      // then set height from that
      sblw = node->w * sblh / node->h;

      // then cap off minimum and maximum of other dimension
      bool capped = false;
      if (node->min_w != ACGL_GUI_DIM_NONE && sblw < minw) {
        sblw = minw;
        capped = true;
      }
      if (node->max_w != ACGL_GUI_DIM_NONE && sblw > maxw) {
        sblw = maxw;
        capped = true;
      }

      // if we did cap, we need to re-set the fill to something maybe non-filled
      if (capped) {
        sblh = sblw * node->h / node->w;
      }
    } else {
      // yes I can't believe I'm using a goto either, but this prevents a lot of code duplication
      // no actually I'm too lazy to restructure these if statements lol.
      goto __ACGL_gui_node_render_set_constant_size;
    }
  } else {
__ACGL_gui_node_render_set_constant_size:
    // we can width and height independently!
    // first, get the baseline w and h
    if (node->node_type & ACGL_GUI_NODE_FILL_W) {
      sblw = (ACGL_gui_pos_t)location.w;
    } else {
      if (node->w_frac) {
        sblw = node->w * location.w;
      } else {
        sblw = node->w;
      }
    }
    if (node->node_type & ACGL_GUI_NODE_FILL_H) {
      sblh = (ACGL_gui_pos_t)location.h;
    } else {
      if (node->h_frac) {
        sblh = node->h * location.h;
      } else {
        sblh = node->h;
      }
    }

    // then cap off at mininum
    if (node->min_w != ACGL_GUI_DIM_NONE && sblw < node->min_w) {
      sblw = node->min_w;
    }
    if (node->min_h != ACGL_GUI_DIM_NONE && sblh < node->min_h) {
      sblh = node->min_h;
    }
    // same with maximum
    if (node->max_w != ACGL_GUI_DIM_NONE && sblw > node->max_w) {
      sblw = node->max_w;
    }
    if (node->max_h != ACGL_GUI_DIM_NONE && sblh < node->max_h) {
      sblh = node->max_h;
    }
  }

  sublocation.w = (int)rint(sblw);
  sublocation.h = (int)rint(sblh);

  // now we can set the position knowing exactly how wide we should be
  if (node->node_type & ACGL_GUI_NODE_FILL_W) {
    sublocation.x = location.x;
  } else {
    if (node->x_frac) {
      sublocation.x = (int)rint((double)location.x + (double)node->x * location.w);
    } else {
      sublocation.x = (int)rint((double)location.x + (double)node->x);
    }
  }

  // 3 possibilities:
  // LEFT: shift right none
  // CENTER: shift right 1/2
  // RIGHT: shift right fully
  // The following if statements, if you trace their logic and assume mutual exclusivity,
  // do this correctly.
  if (!(node->anchor & ACGL_GUI_ANCHOR_RIGHT)) {
    sublocation.x += (location.w - sublocation.w) / 2;
  }
  // if we are anchored to the left, move left again
  if (node->anchor & ACGL_GUI_ANCHOR_LEFT) {
    sublocation.x += (location.w - sublocation.w) / 2;
  }

  if (node->node_type & ACGL_GUI_NODE_FILL_H) {
    sublocation.y = location.y;
  } else {
    if (node->y_frac) {
      sublocation.y = (int)rint((double)location.y + (double)node->y * location.h);
    } else {
      sublocation.y = (int)rint((double)location.y + (double)node->y);
    }
  }

  // if we aren't anchored at the top, move relative coordinates down to the center
  if (!(node->anchor & ACGL_GUI_ANCHOR_TOP)) {
    sublocation.y += (location.h - sublocation.h) / 2;
  }
  // if we are anchored to the bottom, move down again
  if (node->anchor & ACGL_GUI_ANCHOR_BOTTOM) {
    sublocation.y += (location.h - sublocation.h) / 2;
  }

  if (node->needs_update && node->render_callback != NULL) {
    return_val |= (*node->render_callback)(gui->window, sublocation, node->callback_data);
  }

  ACGL_gui_object_t* child = node->last_child;
  while (child != NULL) {
    if (node->needs_update) {
      child->needs_update = true;
    }
    return_val |= ACGL_gui_node_render(gui, child, sublocation);
    child = child->prev_sibling;
  }

  node->needs_update = false;
  SDL_UnlockMutex(node->mutex);

  ENSURES(__ACGL_is_gui_object_t(node));
  ENSURES(__ACGL_is_gui_t(gui));
  return return_val;
}

void ACGL_gui_node_add_child_front(ACGL_gui_object_t* parent, ACGL_gui_object_t* child) {
  REQUIRES(__ACGL_is_gui_object_t(parent));
  REQUIRES(__ACGL_is_gui_object_t(child));

  if (SDL_LockMutex(parent->mutex) != 0) {
    fprintf(stderr, "Could not lock parent mutex in ACGL_gui_node_add_child_front! Things will definitely look wrong. SDL_Error %s\n", SDL_GetError());
    return;
  }

  if (SDL_LockMutex(child->mutex) != 0) {
    fprintf(stderr, "Could not lock child mutex in ACGL_gui_node_add_child_front! Things will definitely look wrong. SDL_Error %s\n", SDL_GetError());
    return;
  }

  ACGL_gui_object_t* first_child = parent->first_child;
  if (first_child == NULL) {
    // there are no nodes in parent (or shouldn't be, at least)
    assert(parent->last_child == NULL);
    parent->first_child = child;
    parent->last_child = child;

    child->parent = parent;
  } else {
    // we should be inserting at an actual front
    assert(first_child->prev_sibling == NULL);
    if (SDL_LockMutex(first_child->mutex) != 0) {
      fprintf(stderr, "Could not lock first_child mutex in ACGL_gui_node_add_child_front! Things will definitely look wrong. SDL_Error %s\n", SDL_GetError());
      return;
    }
    first_child->prev_sibling = child;
    child->next_sibling = first_child;
    child->prev_sibling = NULL;

    parent->first_child = child;
    child->parent = parent;
    SDL_UnlockMutex(first_child->mutex);
  }

  SDL_UnlockMutex(child->mutex);
  SDL_UnlockMutex(parent->mutex);
  ENSURES(__ACGL_is_gui_object_t(parent));
  ENSURES(__ACGL_is_gui_object_t(child));
}

void ACGL_gui_node_add_child_back(ACGL_gui_object_t* parent, ACGL_gui_object_t* child) {
  REQUIRES(__ACGL_is_gui_object_t(parent));
  REQUIRES(__ACGL_is_gui_object_t(child));

  if (SDL_LockMutex(parent->mutex) != 0) {
    fprintf(stderr, "Could not lock parent mutex in ACGL_gui_node_add_child_back! Things will definitely look wrong. SDL_Error %s\n", SDL_GetError());
    return;
  }

  if (SDL_LockMutex(child->mutex) != 0) {
    fprintf(stderr, "Could not lock child mutex in ACGL_gui_node_add_child_back! Things will definitely look wrong. SDL_Error %s\n", SDL_GetError());
    return;
  }

  ACGL_gui_object_t* last_child = parent->last_child;
  if (last_child == NULL) {
    // there are no nodes in parent (or shouldn't be, at least)
    assert(parent->first_child == NULL);
    parent->first_child = child;
    parent->last_child = child;

    child->parent = parent;
  } else {
    // we should be at an actual last child
    assert(last_child->next_sibling == NULL);
    if (SDL_LockMutex(last_child->mutex) != 0) {
      fprintf(stderr, "Could not lock last_child mutex in ACGL_gui_node_add_child_back! Things will definitely look wrong. SDL_Error %s\n", SDL_GetError());
      return;
    }
    last_child->next_sibling = child;
    child->prev_sibling = last_child;
    child->next_sibling = NULL;

    parent->last_child = child;
    child->parent = parent;
    SDL_UnlockMutex(last_child->mutex);
  }

  SDL_UnlockMutex(child->mutex);
  SDL_UnlockMutex(parent->mutex);
  
  ENSURES(__ACGL_is_gui_object_t(parent));
  ENSURES(__ACGL_is_gui_object_t(child));
}

void ACGL_gui_node_remove_child(ACGL_gui_object_t* parent, ACGL_gui_object_t* child) {
  REQUIRES(__ACGL_is_gui_object_t(parent));
  REQUIRES(__ACGL_is_gui_object_t(child));

  if (SDL_LockMutex(parent->mutex) != 0) {
    fprintf(stderr, "Could not lock parent mutex in ACGL_gui_node_remove_child! Things will definitely look wrong. SDL_Error %s\n", SDL_GetError());
    return;
  }

  if (SDL_LockMutex(child->mutex) != 0) {
    fprintf(stderr, "Could not lock child mutex in ACGL_gui_node_remove_child! Things will definitely look wrong. SDL_Error %s\n", SDL_GetError());
    return;
  }

  ACGL_gui_object_t* node = parent->first_child;

  // this loop is necessary to ensure that child actually exists as a child of parent
  while (node != NULL) {
    if (node == child) {
      if (node == parent->first_child) {
        assert(child->prev_sibling == NULL);
        parent->first_child = parent->first_child->next_sibling;
        child->next_sibling = NULL;
        if (node == parent->last_child) {
          parent->last_child = NULL;
        }
      } else if (node == parent->last_child) {
        assert(child->next_sibling == NULL);
        parent->last_child = parent->last_child->prev_sibling;
        child->prev_sibling = NULL;
        // don't have to check for case of single node, already covered in previous statement
      } else {
        // we are somewhere in the middle of the list
        assert (child->next_sibling != NULL && child->prev_sibling != NULL);
        child->next_sibling->prev_sibling = child->prev_sibling;
        child->prev_sibling->next_sibling = child->next_sibling;
        child->prev_sibling = NULL;
        child->next_sibling = NULL;
      }

      // if the node has been added more than once, we have bigger problems
      break;
    }

    node = node->next_sibling;
  }

  child->parent = NULL;

  SDL_UnlockMutex(child->mutex);
  SDL_UnlockMutex(parent->mutex);
  ENSURES(__ACGL_is_gui_object_t(parent));
}

void ACGL_gui_node_remove_all_children(ACGL_gui_object_t* parent) {
  REQUIRES(__ACGL_is_gui_object_t(parent));

  if (SDL_LockMutex(parent->mutex) != 0) {
    fprintf(stderr, "Could not lock parent mutex in ACGL_gui_node_remove_all_children! SDL_Error %s\n", SDL_GetError());
    return;
  }

  ACGL_gui_object_t* child = parent->first_child;
  ACGL_gui_object_t* next_child = child;

  while (next_child != NULL) {
    ASSERT(__ACGL_is_gui_object_t(child));
    next_child = child->next_sibling;
    if (SDL_LockMutex(child->mutex) != 0) {
      fprintf(stderr, "Could not lock child mutex in ACGL_gui_node_remove_all_children! SDL_Error %s\n", SDL_GetError());
      break;
    }
    child->parent = NULL;
    SDL_UnlockMutex(child->mutex);
    child = next_child;
  }

  parent->first_child = NULL;
  parent->last_child = NULL;

  SDL_UnlockMutex(parent->mutex);
  ENSURES(__ACGL_is_gui_object_t(parent));
}

void ACGL_gui_node_destroy_all_children(ACGL_gui_object_t* parent) {
  REQUIRES(__ACGL_is_gui_object_t(parent));

  if (SDL_LockMutex(parent->mutex) != 0) {
    fprintf(stderr, "Could not lock parent mutex in ACGL_gui_node_remove_all_children! SDL_Error %s\n", SDL_GetError());
    return;
  }

  ACGL_gui_object_t* child = parent->first_child;
  ACGL_gui_object_t* next_child = child;

  while (next_child != NULL) {
    next_child = child->next_sibling;
    child->parent = NULL;
    ACGL_gui_node_destroy(child);
    child = next_child;
  }

  parent->first_child = NULL;
  parent->last_child = NULL;

  SDL_UnlockMutex(parent->mutex);
  ENSURES(__ACGL_is_gui_object_t(parent));
}

void ACGL_gui_node_destroy(ACGL_gui_object_t* node) {
  REQUIRES(__ACGL_is_gui_object_t(node));
  // REQUIRES: no thread contention. can't quite enforce that explicitly b/c destroying mutex here :L

  // Then, recursively free all children. This is safe because
  // we do this before destroying our own node, so all child calls
  // will still have a proper parent to remove themselves from
  ACGL_gui_node_destroy_all_children(node);
  
  // Then free data related to the node
  if (node->mutex != NULL) {
    SDL_DestroyMutex(node->mutex);
    node->mutex = NULL;
  }
  if (node->callback_data != NULL) {
    if (node->destroy_callback != NULL) {
      (*node->destroy_callback)(node->callback_data);
    }
    node->callback_data = NULL;
  }
  // node->renderer is shared, don't free it
  free(node);
  // make sure to set to NULL on the outside, don't want any dangling refrences
}
