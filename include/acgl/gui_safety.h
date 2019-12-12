#ifndef ACGL_GUI_SAFETY_H
#define ACGL_GUI_SAFETY_H

#include "gui.h"
#include "contracts.h"

// Checks if a gui_t is well-formed
bool __ACGL_is_gui_t(ACGL_gui_t* gui);

// Checks if the list of ACGL_gui_object_t's formed by first_child
// to last_child is acyclic and well-formed. Uses the Tortoise-Hare algorithm.
bool __ACGL_is_acyclic_list(ACGL_gui_object_t *object);

// Two methods to check if a gui_object_t tree is well-formed
bool __ACGL_tree_contains_node(ACGL_gui_object_t *root, ACGL_gui_object_t *object);
bool __ACGL_is_acyclic_tree(ACGL_gui_object_t *object);

// Checks if a gui_object_t is well-formed
bool __ACGL_is_gui_object_t(ACGL_gui_object_t* object);

#endif // ACGL_GUI_SAFETY_H
