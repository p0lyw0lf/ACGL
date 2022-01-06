#include "gui_safety.h"

bool __ACGL_is_gui_t(ACGL_gui_t* gui) {
    if (gui == NULL) {
        fprintf(stderr, "Error! NULL ACGL_gui_t\n");
        return false;
    }

    if (gui->root != NULL && !__ACGL_is_gui_object_t(gui->root)) {
        fprintf(stderr, "Error! NULL ACGL_gui_t->root\n");
        return false;
    }

    if (gui->window == NULL) {
        fprintf(stderr, "Error! NULL ACGL_gui_t->window\n");
        return false;
    }

    return true;
}

// Checks if the list of ACGL_gui_object_t's formed by first_child
// to last_child is acyclic and well-formed. Uses the Tortoise-Hare algorithm.
bool __ACGL_is_acyclic_list(ACGL_gui_object_t *object) {
    REQUIRES(object != NULL);
    ACGL_gui_object_t *tort = object->first_child;
    ACGL_gui_object_t *hare = object->first_child;

    if (tort != NULL && tort->prev_sibling != NULL) {
        // Invalid link
        return false;
    }

    while (hare != object->last_child && hare != NULL) {
        // Check for invalid links ahead of tort every time it steps
        if (tort->next_sibling != NULL && tort->next_sibling->prev_sibling != tort) {
            return false;
        }
        if (tort->parent != object) {
            return false;
        }
        tort = tort->next_sibling;
        hare = hare->next_sibling;
        if (hare == NULL) {
            // Reached end without reaching last_child
            return false;
        }
        hare = hare->next_sibling;
        // If the Tortoise caught up to the hare, there is a cycle
        if (tort == hare) {
            return false;
        }
    }

    ASSERT(hare == NULL || hare == object->last_child);
    // If we break cleanly (hare reaches end before tortoise), then
    // there is no cycle
    // Still need to check that we ended in the correct place:
    if (hare != NULL && hare->next_sibling != NULL) {
        return false;
    }

    // Check remaining list for invalid links
    while (tort != object->last_child && tort != NULL) {
        if (tort->next_sibling != NULL && tort->next_sibling->prev_sibling != tort) {
            return false;
        }
        if (tort->parent != object) {
            return false;
        }
    }

    return object->last_child == NULL || object->last_child->parent == object;
}

bool __ACGL_tree_contains_node(ACGL_gui_object_t *root, ACGL_gui_object_t *object) {
    REQUIRES(root != NULL && object != NULL);
    REQUIRES(__ACGL_is_acyclic_list(root));

    for (ACGL_gui_object_t *p = root->first_child; 
         p != NULL;
         p = p->next_sibling) {
        if (p == object) {
            return true;
        }
        if (__ACGL_tree_contains_node(p, object)) return true;
    }

    return false;
}

bool __ACGL_is_acyclic_tree(ACGL_gui_object_t *object) {
    REQUIRES(object != NULL);

    if (!__ACGL_is_acyclic_list(object)) {
        fprintf(stderr, "Error! ACGL_gui_object_t has cyclic children!\n");
        return false;
    }

    if (__ACGL_tree_contains_node(object, object)) {
        return false;
    }

    for (ACGL_gui_object_t *p = object->first_child;
         p != NULL;
         p = p->next_sibling) {
        if (!__ACGL_is_acyclic_tree(p)) {
            return false;
        }
    }

    return true;
}

bool __ACGL_is_gui_object_t(ACGL_gui_object_t* object) {
    if (object == NULL) {
        fprintf(stderr, "Error! NULL ACGL_gui_object_t\n");

        return false;
    }

    if (object->mutex == NULL) {
        fprintf(stderr, "Error! NULL ACGL_gui_object_t->mutex\n");
        return false;
    }
    
    if (!__ACGL_is_acyclic_tree(object)) {
        fprintf(stderr, "Error! ACGL_gui_object_t contains a cycle!\n");
        return false;
    }

    return true;
}
