#include "gui.h"
#include "gui_safety.h"
#include "contracts.h"

bool ACGL_gui_force_update(ACGL_gui_t* gui) {
    REQUIRES(__ACGL_is_gui_t(gui));
   
    bool old_update = gui->root->needs_update;
    gui->root->needs_update = true;
    return !old_update;
}

bool ACGL_gui_render(ACGL_gui_t* gui) {
  REQUIRES(__ACGL_is_gui_t(gui));

  int w, h;
  SDL_GetRendererOutputSize(gui->renderer, &w, &h);
  SDL_Rect location = {0, 0, w, h};

  return ACGL_gui_node_render(gui->root, location);
}

bool ACGL_gui_blank_callback(SDL_Renderer* renderer, SDL_Rect location, void* data) {
  return false;
}

ACGL_gui_t* ACGL_gui_init(SDL_Renderer* renderer) {
  assert(renderer != NULL);

  ACGL_gui_t* gui = (ACGL_gui_t*)malloc(sizeof(ACGL_gui_t));
  if (gui == NULL) {
    fprintf(stderr, "Error! Could not malloc gui in ACGL_gui_init\n");
    return NULL;
  }
  gui->renderer = renderer;
  gui->root = ACGL_gui_node_init(renderer, &ACGL_gui_blank_callback, NULL);

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

  ACGL_gui_node_destroy(gui->root);
  gui->root = NULL;

  // don't destroy renderer, save it for main code to handle
  // example: we could just we switching away from ACGL
  gui->renderer = NULL;

  free(gui);
}

ACGL_gui_object_t* ACGL_gui_node_init(SDL_Renderer* renderer, ACGL_gui_callback_t callback, void* data) {
  assert(renderer != NULL);

  ACGL_gui_object_t* node = (ACGL_gui_object_t*)malloc(sizeof(ACGL_gui_object_t));
  if (node == NULL) {
    fprintf(stderr, "Error! could not malloc node in ACGL_gui_node_init\n");
    return NULL;
  }

  node->renderer = renderer;
  node->mutex = SDL_CreateMutex();
  if (node->mutex == NULL) {
    fprintf(stderr, "Could not create mutex in ACGL_gui_node_init! SDL Error: %s\n", SDL_GetError());
    free(node);
    return NULL;
  }

  node->callback = callback;
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

bool ACGL_gui_node_render(ACGL_gui_object_t* node, SDL_Rect location) {
  REQUIRES(__ACGL_is_gui_object_t(node));

  bool return_val = false;

  // compute rectangle that we are supposed to draw in
  SDL_Rect sublocation = location;

  if (node->node_type & ACGL_GUI_NODE_PRESERVE_ASPECT) {
    if ( (node->node_type & ACGL_GUI_NODE_FILL_W) && !(node->node_type & ACGL_GUI_NODE_FILL_H) ) {
      // fill all the available space first
      sublocation.w = location.w;

      // cap off minimum and maximum
      if (node->min_w != ACGL_GUI_DIM_NONE && sublocation.w < node->min_w) {
        sublocation.w = node->min_w;
      }
      if (node->max_w != ACGL_GUI_DIM_NONE && sublocation.w > node->max_w) {
        sublocation.w = node->max_w;
      }

      // then set height from that
      sublocation.h = sublocation.w * node->h / node->w;

      // then cap off minimum and maximum of other dimension
      bool capped = false;
      if (node->min_h != ACGL_GUI_DIM_NONE && sublocation.h < node->min_h) {
        sublocation.h = node->min_h;
        capped = true;
      }
      if (node->max_h != ACGL_GUI_DIM_NONE && sublocation.h > node->max_h) {
        sublocation.h = node->max_h;
        capped = true;
      }

      // if we did cap, we need to re-set the fill to something maybe non-filled
      if (capped) {
        sublocation.w = sublocation.h * node->w / node->h;
      }

    } else if ( !(node->node_type & ACGL_GUI_NODE_FILL_W) && (node->node_type & ACGL_GUI_NODE_FILL_H) ) {
      // fill all the available space first
      sublocation.h = location.h;

      // cap off minimum and maximum
      if (node->min_h != ACGL_GUI_DIM_NONE && sublocation.h < node->min_h) {
        sublocation.h = node->min_h;
      }
      if (node->max_h != ACGL_GUI_DIM_NONE && sublocation.h > node->max_h) {
        sublocation.h = node->max_h;
      }

      // then set height from that
      sublocation.w = sublocation.h * node->w / node->h;

      // then cap off minimum and maximum of other dimension
      bool capped = false;
      if (node->min_w != ACGL_GUI_DIM_NONE && sublocation.w < node->min_w) {
        sublocation.w = node->min_w;
        capped = true;
      }
      if (node->max_w != ACGL_GUI_DIM_NONE && sublocation.w > node->max_w) {
        sublocation.w = node->max_w;
        capped = true;
      }

      // if we did cap, we need to re-set the fill to something maybe non-filled
      if (capped) {
        sublocation.h = sublocation.w * node->h / node->w;
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
      sublocation.w = location.w;
    } else {
      if (node->w_frac) {
        sublocation.w = node->w * location.w;
      } else {
        sublocation.w = node->w;
      }
    }
    if (node->node_type & ACGL_GUI_NODE_FILL_H) {
      sublocation.h = location.h;
    } else {
      if (node->h_frac) {
        sublocation.h = node->h * location.h;
      } else {
        sublocation.h = node->h;
      }
    }

    // then cap off at mininum
    if (node->min_w != ACGL_GUI_DIM_NONE && sublocation.w < node->min_w) {
      sublocation.w = node->min_w;
    }
    if (node->min_h != ACGL_GUI_DIM_NONE && sublocation.h < node->min_h) {
      sublocation.h = node->min_h;
    }
    // same with maximum
    if (node->max_w != ACGL_GUI_DIM_NONE && sublocation.w > node->max_w) {
      sublocation.w = node->max_w;
    }
    if (node->max_h != ACGL_GUI_DIM_NONE && sublocation.h < node->max_h) {
      sublocation.h = node->max_h;
    }
  }

  // now we can set the position knowing exactly how wide we should be
  if (node->node_type & ACGL_GUI_NODE_FILL_W) {
    sublocation.x = location.x;
  } else {
    if (node->x_frac) {
      sublocation.x = location.x + node->x * location.w;
    } else {
      sublocation.x = location.x + node->x;
    }
  }

  // if we aren't anchored to the right, move relative coordinates to the left.
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
      sublocation.y = location.y + node->y * location.h;
    } else {
      sublocation.y = location.x + node->x;
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

  if (node->needs_update) {
    return_val |= (*node->callback)(node->renderer, sublocation, node->callback_data);
  }

  ACGL_gui_object_t* child = node->last_child;
  while (child != NULL) {
    if (node->needs_update) {
      child->needs_update = true;
    }
    return_val |= ACGL_gui_node_render(child, sublocation);
    child = child->prev_sibling;
  }

  node->needs_update = false;
  SDL_UnlockMutex(node->mutex);

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
    fprintf(stderr, "Could not lock parent mutex in ACGL_gui_node_remove_all_children! Things will definitely look wrong. SDL_Error %s\n", SDL_GetError());
    return;
  }

  ACGL_gui_object_t* child = parent->first_child;
  ACGL_gui_object_t* next_child = child;

  while (next_child != NULL) {
    next_child = child->next_sibling;
    ACGL_gui_node_destroy(child);
    child = next_child;
  }

  parent->first_child = NULL;
  parent->last_child = NULL;

  SDL_UnlockMutex(parent->mutex);
  ENSURES(__ACGL_is_gui_object_t(parent));
}

void ACGL_gui_node_destroy(ACGL_gui_object_t* node) {
  REQUIRES(__ACGL_is_gui_object_t(parent));

  ACGL_gui_node_remove_all_children(node);
  free(node);
  // make sure to set to NULL on the outside, don't want any dangling refrences
}
