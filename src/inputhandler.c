#include "inputhandler.h"

ACGL_ih_keybinds_t* ACGL_ih_init_keybinds(const SDL_Scancode keycodes[], const size_t keycodes_size) {
	ACGL_ih_keybinds_t* keybinds = (ACGL_ih_keybinds_t*)malloc(sizeof(ACGL_ih_keybinds_t));
	if (keybinds == NULL) {
		fprintf(stderr, "Error! could not malloc keybinds in ACGL_ih__init_keybinds\n");
		return NULL;
	}

	keybinds->keycodes = (SDL_Scancode*)malloc(keycodes_size * sizeof(SDL_Scancode));
	for (size_t i=0; i<keycodes_size; ++i) {
		keybinds->keycodes[i] = keycodes[i];
	}
	keybinds->keycodes_size = keycodes_size;

	return keybinds;
}

ACGL_ih_eventdata_t* ACGL_ih_init_eventdata(const size_t keycodes_size) {
	ACGL_ih_eventdata_t* medata = (ACGL_ih_eventdata_t*)malloc(sizeof(ACGL_ih_eventdata_t));
	if (medata == NULL) {
		fprintf(stderr, "Error! could not malloc eventdata in ACGL_ih__init_eventdata\n");
		return NULL;
	}

	medata->keyCallbacks = (ACGL_ih_callback_node_t**)malloc(sizeof(ACGL_ih_callback_node_t*)*keycodes_size);
	for (size_t i=0; i<keycodes_size; ++i) {
		medata->keyCallbacks[i] = NULL;
	}
	medata->keyCallbacks_size = keycodes_size;
	medata->windowCallbacks = NULL;

	return medata;
}

void ACGL_ih_deinit_keybinds(ACGL_ih_keybinds_t* keybinds) {
	if (keybinds == NULL) {
		fprintf(stderr, "Error! cannot deinit NULL pointer in ACGL_deinit_keybinds\n");
		return;
	}

	if (keybinds->keycodes != NULL) {
		free(keybinds->keycodes);
		keybinds->keycodes = NULL;
	}

	free(keybinds);
}

void ACGL_ih_deinit_eventdata(ACGL_ih_eventdata_t* medata) {
	if (medata == NULL) {
		fprintf(stderr, "Error! cannot deinit NULL pointer in ACGL_ih_deinit_eventdata\n");
		return;
	}
	ACGL_ih_callback_node_t* ptr;
	ACGL_ih_callback_node_t* next_ptr;

	if (medata->keyCallbacks != NULL) {
		for (Uint16 i=0; i<medata->keyCallbacks_size; ++i) {
			ptr = medata->keyCallbacks[i];
			medata->keyCallbacks[i] = NULL;

			while (ptr != NULL) {
				next_ptr = ptr->next;
				ptr->next = NULL;
				free(ptr);
				ptr = next_ptr;
			}
		}
		free(medata->keyCallbacks);
	}

	ptr = medata->windowCallbacks;
	medata->windowCallbacks = NULL;
	while (ptr != NULL) {
		next_ptr = ptr->next;
		ptr->next = NULL;
		free(ptr);
		ptr = next_ptr;
	}

	free(medata);
}

void ACGL_ih_register_keyevent(ACGL_ih_eventdata_t* medata, Uint16 keytype, ACGL_ih_callback_t callback, void* data) {
	if (keytype >= medata->keyCallbacks_size) {
		// unsigned, no need to check for below zero
		fprintf(stderr, "Error! keytype %d is out-of-bounds for length %llu keyCallbacks in ACGL_ih_register_keyevent\n", keytype, medata->keyCallbacks_size);
		return;
	}

	ACGL_ih_callback_node_t* new_node = malloc(sizeof(ACGL_ih_callback_node_t));
	new_node->callback = callback;
	new_node->data = data;
	// appends to front of list, is the fastest
	// + order shouldn't matter anyway
	new_node->next = medata->keyCallbacks[keytype];
	medata->keyCallbacks[keytype] = new_node;
}

void ACGL_ih_register_windowevent(ACGL_ih_eventdata_t* medata, ACGL_ih_callback_t callback, void* data) {
	ACGL_ih_callback_node_t* new_node = malloc(sizeof(ACGL_ih_callback_node_t));
	new_node->callback = callback;
	new_node->data = data;
	// appends to front of list, is the fastest
	// + order shouldn't matter anyway
	new_node->next = medata->windowCallbacks;
	medata->windowCallbacks = new_node;
}

void ACGL_ih_deregister_keyevent(ACGL_ih_eventdata_t* medata, Uint16 keytype, ACGL_ih_callback_t callback) {
	if (keytype >= medata->keyCallbacks_size) {
		// unsigned, no need to check for below zero
		fprintf(stderr, "Error! keytype %d is out-of-bounds for length %llu keyCallbacks in ACGL_ih_deregister_keyevent\n", keytype, medata->keyCallbacks_size);
		return;
	}

	ACGL_ih_callback_node_t* ptr = medata->keyCallbacks[keytype];
	ACGL_ih_callback_node_t* prev = NULL;

	while (ptr != NULL) {
		if (ptr->callback == callback) {
			// remove the node
			if (prev == NULL) {
				// we at the front of the list
				ptr = medata->keyCallbacks[keytype];
				medata->keyCallbacks[keytype] = ptr->next;
				ptr->next = NULL;
				free(ptr);
				// keep going just in case function was registered twice
				ptr = medata->keyCallbacks[keytype]->next;
				continue;
			} else {
				prev->next = ptr->next;
				ptr->next = NULL;
				free(ptr);
				// keep going just in case function was registered twice
				ptr = prev->next;
				continue;
			}
		}

		prev = ptr;
		ptr = ptr->next;
	}
}

void ACGL_ih_deregister_windowevent(ACGL_ih_eventdata_t* medata, ACGL_ih_callback_t callback) {
	ACGL_ih_callback_node_t* ptr = medata->windowCallbacks;
	ACGL_ih_callback_node_t* prev = NULL;

	while (ptr != NULL) {
		if (ptr->callback == callback) {
			// remove the node
			if (prev == NULL) {
				// we at the front of the list
				ptr = medata->windowCallbacks;
				medata->windowCallbacks = ptr->next;
				ptr->next = NULL;
				free(ptr);
				// keep going just in case function was registered twice
				ptr = medata->windowCallbacks->next;
				continue;
			} else {
				prev->next = ptr->next;
				ptr->next = NULL;
				free(ptr);
				// keep going just in case function was registered twice
				ptr = prev->next;
				continue;
			}
		}

		prev = ptr;
		ptr = ptr->next;
	}
}

bool ACGL_ih_handle_keyevent(SDL_Event event, ACGL_ih_keybinds_t* keybinds, ACGL_ih_eventdata_t* medata) {
	bool calledSomething = false;

	assert(keybinds->keycodes_size == medata->keyCallbacks_size);

	for (Uint16 i=0; i<medata->keyCallbacks_size; ++i) {
		if (event.key.keysym.scancode == keybinds->keycodes[i]) {
			ACGL_ih_callback_node_t* ptr = medata->keyCallbacks[i];

			while (ptr != NULL) {
				ptr->callback(ptr->data, event);
				ptr = ptr->next;
				calledSomething = true;
			}
		}
	}

	return calledSomething;
}
bool ACGL_ih_handle_windowevent(SDL_Event event, ACGL_ih_eventdata_t* medata) {
	bool calledSomething = false;

	ACGL_ih_callback_node_t* ptr = medata->windowCallbacks;
	while (ptr != NULL) {
		ptr->callback(ptr->data, event);
		ptr = ptr->next;
		calledSomething = true;
	}

	return calledSomething;
}
