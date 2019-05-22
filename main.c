#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <linux/input.h>

#include <wayland-client.h>
#include "xdg-shell-protocol.c"         // NOTE(Felix): These files are created on compile-time
#include "xdg-shell-client-protocol.h"  // NOTE(Felix): Have a look at the makefile for further notes

#define RESOLUTION_WIDTH 1280
#define RESOLUTION_HEIGHT 720


static void *Data = 0; // NOTE(Felix): Pointer to Image Buffer
static uint64_t Stride = sizeof(uint32_t)*RESOLUTION_WIDTH;
static int32_t GlobalRunning = 1;

// NOTE(Felix): We have to use Listeners with corresponding callback functions
// We save some global settings / references here, as the callback functions have set signatures
static struct wl_display *Display = 0;
static struct wl_compositor *Compositor = 0;
static struct wl_shm *Shm = 0;
static struct wl_surface *Surface = 0;
static struct wl_callback *FrameCallback = 0;
static struct wl_buffer *Buffer = 0;
static struct wl_seat *Seat = 0;
static struct xdg_wm_base *Base;
static struct xdg_toplevel *XdgToplevel = 0;


// NOTE(Felix): This function is used to print received registry events
// as well as save reference to the global objects / singletons.
static void
CallbackRegistryGlobalAnnounce(void *Data, struct wl_registry *Registry,
                               uint32_t Id,
                               const char *Interface, uint32_t Version)
{
	fprintf(stderr, "Registry event for %s with ID %d received.\n", Interface, Id);
	if (strcmp(Interface, wl_compositor_interface.name) == 0)
	{
		Compositor = wl_registry_bind(Registry,
		                              Id,
		                              &wl_compositor_interface,
		                              1);
	}
	else if (strcmp(Interface, wl_shm_interface.name) == 0)
	{
		Shm = wl_registry_bind(Registry,
		                       Id,
		                       &wl_shm_interface,
		                       1);
	}
	else if (strcmp(Interface, xdg_wm_base_interface.name) == 0)
	{
		Base = wl_registry_bind(Registry,
		                        Id,
		                        &xdg_wm_base_interface,
		                        1);
	}
	else if (strcmp(Interface, wl_seat_interface.name) == 0)
	{
		Seat = wl_registry_bind(Registry,
								Id,
								&wl_seat_interface,
								1);
	}
}

static void
CallbackRegistryGlobalRemove(void *Data, struct wl_registry *Registry,
                             uint32_t Id)
{
	fprintf(stderr, "Registry remove call received.\n");
}

// NOTE(Felix): Callback settings for the functions above
// Get global objects / singletons
static const struct wl_registry_listener GlobalRegistryListener = {
	.global = CallbackRegistryGlobalAnnounce,
	.global_remove = CallbackRegistryGlobalRemove
};

static void CallbackXdgSurfaceConfigure(void *Data, 
                                        struct xdg_surface *XdgSurface,
                                        uint32_t Id)
{
	// NOTE(Felix): Configure surface
	xdg_surface_ack_configure(XdgSurface, Id);
}

// NOTE(Felix): Callback for the xdg_surface
static const struct xdg_surface_listener XdgSurfaceListener = {
	.configure = CallbackXdgSurfaceConfigure
};

static void
CallbackXdgToplevelConfigure()
{ 
	// NOTE(Felix): Init stuff here
}

static void
CallbackXdgToplevelClose(void *Data,
                         struct xdg_toplevel *XdgToplevel)
{
	GlobalRunning = 0;
}

// NOTE(Felix): Callback settings for the xdg toplevel
static const struct xdg_toplevel_listener XdgToplevelListener = {
	.configure = CallbackXdgToplevelConfigure,
	.close = CallbackXdgToplevelClose
};

// NOTE(Felix): This function does the actual drawing for the window
static void
PaintPixels(void)
{
	// TODO(Felix): Do this smarter / do something cooler than this
	static uint32_t offset = 0;
	for (uint32_t Y = 0; Y < RESOLUTION_HEIGHT; ++Y)
	{
		uint32_t *PixelY = Data + Y*Stride;
		for (uint32_t X = 0; X < RESOLUTION_WIDTH; ++X)
		{
			uint32_t *PixelX = PixelY+X;

			*PixelX = 0xFF0000FF;
			*PixelX += offset;
		}
	}

	++offset;
}

// NOTE(Felix): Protoype
static const struct wl_callback_listener FrameListener;
static void
CallbackFrameDraw(void *Data, struct wl_callback *Callback, uint32_t time)
{   
	wl_callback_destroy(Callback);
	wl_surface_damage(Surface, 0, 0, RESOLUTION_WIDTH, RESOLUTION_HEIGHT);
	PaintPixels();
	FrameCallback = wl_surface_frame(Surface);
	wl_surface_attach(Surface, Buffer, 0, 0);
	wl_callback_add_listener(FrameCallback, &FrameListener, 0);
	wl_surface_commit(Surface);
}

// NOTE(Felix): Callback settings for a custom Listener,
// used for the "frame" of the surface (actual pixels)
static const struct wl_callback_listener FrameListener = {
	.done = CallbackFrameDraw
};

static void
CallbackKeyboardKeymap(void *Data, struct wl_keyboard *Keyboard, uint32_t Format, int32_t Fd, uint32_t Size)
{
	// NOTE(Felix): Keyboard mapping
}

static void
CallbackKeyboardEnter(void *Data, struct wl_keyboard *Keyboard, uint32_t Serial, struct wl_surface *Surface, struct wl_array *Keys)
{
	// NOTE(Felix): Enter event for a surface
}
 
static void
CallbackKeyboardLeave(void *Data, struct wl_keyboard *Keyboard, uint32_t Serial, struct wl_surface *Surface)
{
	// NOTE(Felix): Leave event for a surface
}

static void
CallbackKeyboardKey(void *Data, struct wl_keyboard *Keyboard, 
					uint32_t Serial, uint32_t Time, uint32_t Key, uint32_t State)
{
	// NOTE(Felix): Use linux/input.h keycodes
	if (Key == KEY_ESC && State == WL_KEYBOARD_KEY_STATE_PRESSED)
	{
		GlobalRunning = 0;
	}
	else if (State == WL_KEYBOARD_KEY_STATE_PRESSED) 
	{
		fprintf(stderr, "Key with keycode %i was just pressed.\n", Key);
	} 
	else if (State == WL_KEYBOARD_KEY_STATE_RELEASED) 
	{
		fprintf(stderr, "Key with keycode %i was just released.\n", Key);
	}

}

static void
CallbackKeyboardModifiers(void *Data, struct wl_keyboard *Keyboard, uint32_t Serial, 
						  uint32_t ModsDepressed, uint32_t ModsLatched, uint32_t ModsLocked, uint32_t Group)
{
	// NOTE(Felix): Modifiers and group state(?)
}

static void
CallbackKeyboardRepeatInfo(void *Data, struct wl_keyboard *Keyboard, int32_t Rate, int32_t Delay)
{
	// NOTE(Felix): Repeat rate and delay
}

// NOTE(Felix): Keyboard callbacks
static const struct wl_keyboard_listener KeyboardListener = {
	// NOTE(Felix): These all need to be set :(
	.keymap = CallbackKeyboardKeymap,
	.enter = CallbackKeyboardEnter,
	.leave = CallbackKeyboardLeave,
	.key = CallbackKeyboardKey,
	.modifiers = CallbackKeyboardModifiers,
	.repeat_info = CallbackKeyboardRepeatInfo
};

int 
main(void)
{
	// NOTE(Felix): Connect to Server
	Display = wl_display_connect(0);
	if (!Display) 
	{
		fprintf(stderr, "Could not connect to Wayland display.\n");
		return (-1);
	}

	// NOTE(Felix): Add listeners to receive global objects
	struct wl_registry *Registry = wl_display_get_registry(Display);
	wl_registry_add_listener(Registry, &GlobalRegistryListener, 0);
	wl_display_dispatch(Display);
	wl_display_roundtrip(Display);
	if (!Compositor)
	{
		fprintf(stderr, "Cannot find compositor.\n");
		return (-1);
	}
	fprintf(stderr, "Found compositor.\n");
	if (!Shm)
	{
		fprintf(stderr, "Cannot find global shared memory object.\n");
		return (-1);
	}
	fprintf(stderr, "Found global shared memory object.\n");
	if (!Base)
	{
		fprintf(stderr, "Cannot find global xdg_wm_base object.\n");
		return (-1);
	}
	fprintf(stderr, "Found global xdg_wm_base object.\n");
	if (!Seat)
	{
		fprintf(stderr, "Cannot find global wl_seat object.\n");
		return (-1);
	}
	fprintf(stderr, "Found global wl_seat object.\n");


	// NOTE(Felix): Get input device
	// TODO(Felix): Add a listener to get the available inputs
	struct wl_keyboard *Keyboard = wl_seat_get_keyboard(Seat);
	if (!Keyboard)
	{
		fprintf(stderr, "Cannot get the keyboard interface.\n");
		return (-1);
	}
	wl_keyboard_add_listener(Keyboard, &KeyboardListener, WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP);
	fprintf(stderr, "Connected to keyboard.\n");


	// NOTE(Felix): Create surface from compositor - is used to draw stuff on it
	Surface = wl_compositor_create_surface(Compositor);
	if (!Surface)
	{
		fprintf(stderr, "Could not create surface.\n");
		return (-1);
	}
	fprintf(stderr, "Created surface.\n");


	// NOTE(Felix): Memory to map the pool to
	uint64_t Size = sizeof(uint32_t)*RESOLUTION_WIDTH*RESOLUTION_HEIGHT;
	char const *SharedMemoryName = "/MySharedMemory";
	int Fd = shm_open(SharedMemoryName, O_RDWR|O_CREAT, S_IRWXU);
	if (Fd == -1)
	{
		fprintf(stderr, "shm_open error.\n");
		return(-1);
	}
	ftruncate(Fd, Size);
	Data = mmap(0, Size, PROT_READ|PROT_WRITE, MAP_SHARED, Fd, 0);
	if (Data == MAP_FAILED)
	{
		fprintf(stderr, "mmap failed.\n");
		return (-1);
	}
	struct wl_shm_pool *MemoryPool = wl_shm_create_pool(Shm, Fd, Size);
	if (!MemoryPool)
	{
		fprintf(stderr, "Unable to create shared memory pool.\n");
		return (-1);
	}
	fprintf(stderr, "Created shared memory pool.\n");


	// NOTE(Felix): Create wl_buffer from memory pool, with specific memory layout
	// TODO(Felix): Get format with wl_shm_add_listener();
	Buffer = wl_shm_pool_create_buffer(MemoryPool,
	                                   0,
	                                   RESOLUTION_WIDTH,
	                                   RESOLUTION_HEIGHT,
	                                   Stride,
	                                   WL_SHM_FORMAT_ARGB8888);
	if (!Buffer)
	{
		fprintf(stderr, "Unable to create buffer from shared memory.\n");
		return (-1);
	}
	fprintf(stderr, "Created buffer\n");


	// NOTE(Felix): Use Xdg surface(Created from Surface, used by the compositor for some reason)
	struct xdg_surface *XdgSurface = xdg_wm_base_get_xdg_surface(Base, Surface);
	XdgToplevel = xdg_surface_get_toplevel(XdgSurface);
	xdg_surface_add_listener(XdgSurface, &XdgSurfaceListener, 0);
	xdg_toplevel_add_listener(XdgToplevel, &XdgToplevelListener, 0);

	// NOTE(Felix): Trigger callbacks
	wl_surface_commit(Surface);
	wl_display_roundtrip(Display);

	// NOTE(Felix): Setup Frame listeners (for notification to redraw)
	FrameCallback = wl_surface_frame(Surface);
	wl_callback_add_listener(FrameCallback, &FrameListener, 0);

	// NOTE(Felix): Attach buffer to surface
	wl_surface_attach(Surface, Buffer,
	                  0, 0);
	wl_surface_commit(Surface);

	// NOTE(Felix): Main loop, dispatch also triggers redraw events
	while (wl_display_dispatch(Display) != -1 && GlobalRunning) 
	{
		// TODO(Felix): Process events - look into "wl_event_queue"
	}

	// NOTE(Felix): Cleanup
	munmap(Data, Size);
	if (shm_unlink(SharedMemoryName) == -1)
	{
		fprintf(stderr, "shm_unlink error.\n");
	}
	xdg_toplevel_destroy(XdgToplevel);
	xdg_surface_destroy(XdgSurface);
	wl_seat_release(Seat);
	wl_surface_destroy(Surface);
	wl_display_disconnect(Display);
	return (0);
}
