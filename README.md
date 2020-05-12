# ACGL
[**Another Custom GUI Library**](https://github.com/p0lyw0lf/ACGL)

ACGL wraps the popular [SDL](https://www.libsdl.org/) framework in a slightly 
higher-level API.

The goals of ACGL are straightforward:
* Be small and fast
* Use pure, standard C constructs
* Give the users control

The first bullet means there won't be as many features. The second bullet means 
C++ classes, while indeed extremely useful, are not directly supported by this 
library. The third bullet, and the limitations of SDL, mean that almost no 
direct graphics will be done by this library, only content layout.

You are supposed to read the headers to figure this out; documentation beyond 
this file currently does not exist.

## Features

- [x] Anchor points
- [x] Dynamic width elements
- [x] Re-bindable keys
- [x] Callback-based event handling
- [ ] Mouse move/click events
- [x] Easy worker thread support
- [ ] Easy SDL initialization

## Rendering structure

ACGL uses the concept of "views" to determine how content is laid out. ACGL 
requires an `SDL_Renderer*`, preferably the one belonging to a window, to get 
started. From there, you make an `ACGL_gui_t*` which starts a rendering tree in 
it's `ACGL_gui_object_t* root` property. Each `ACGL_gui_object_t*` stores a 
pointer to its parent and a set of pointers that allow it and its children to 
act like a doubly linked list.

Each `ACGL_gui_object_t*` has a `ACGL_gui_callback_t` callback function that 
gets passed the renderer, the `SDL_Rect` where it is supposed to draw itself, 
and a `void*` to any custom data it needs. Parents draw themselves, then 
iterate rendering from their last to first child. This structure acts like the 
traditional layer system in most art software.

The default node, when created, fills up all the available width and height and 
is anchored at the center. This is, in fact, the properties of the default 
`root` node. You can (and should) change these properties when adding new 
children nodes. Look at the header files for more information on how to do 
that, exactly.

## Input structure

Inputs are done with callbacks. At the program initialization, you should 
provide a list of `SDL_Scancode`s you are going to register callbacks to. You 
should also register the callback functions at that time. Then, each iteration 
of the loop where you `SDL_PollEvent`, you should check before passing events 
to the `ACGL_ih_handle_*` functions.

-----

My personal goal for making this project was to have something that I knew how 
to use, because I had written it. Yes, I could've used [LÖVE](https://love2d.org/) 
or some other user-friendly library to make the game I wanted, but doing this 
instead was very instructional.

Yes I know this project isn't great, thanks for pointing that out. Don't use it 
then.
