#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <wayland-client.h>
//#include <wayland-client-protocol.h>
#include "xdg-shell-protocol.c"
#include "xdg-shell-client-protocol.h"

#define RESOLUTION_WIDTH 1280
#define RESOLUTION_HEIGHT 720

struct wl_display *Display = 0;
struct wl_compositor *Compositor = 0;
struct wl_shm *Shm = 0;
struct xdg_wm_base *Base;
struct xdg_toplevel *XdgToplevel = 0;
static int32_t GlobalRunning = 1;

static void
global_registry_handler(void *Data, struct wl_registry *Registry,
						uint32_t Id,
						const char *Interface, uint32_t Version)
{
	fprintf(stderr, "Registry event for %s with ID %d received.\n", Interface, Id);
	// NOTE(Felix): Retreive global objects
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
}

static void
global_registry_remover(void *Data, struct wl_registry *Registry,
						uint32_t Id)
{
	fprintf(stderr, "Registry remove call received.\n");
}

static const struct wl_registry_listener GlobalRegistryListener = {
	global_registry_handler,
	global_registry_remover
};

static void xdg_surface_handle_configure (void *data, 
										  struct xdg_surface *xdg_surface,
										  uint32_t serial)
{
	xdg_surface_ack_configure(xdg_surface, serial);
}

static const struct xdg_surface_listener  xdg_surface_listener = {
	 .configure = xdg_surface_handle_configure
};

static void
noop()
{
}

static void xdg_toplevel_handle_close(void *data,
									  struct xdg_toplevel *xdg_toplevel)
{
	GlobalRunning = 0;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = noop,
	.close = xdg_toplevel_handle_close
};

int 
main(void)
{
	Display = wl_display_connect(0);
	if (!Display) 
	{
		fprintf(stderr, "Could not connect to Wayland display.\n");
		return (-1);
	}

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

	// NOTE(Felix): Create surface - is used to draw stuff on it
	struct wl_surface *Surface = wl_compositor_create_surface(Compositor);
	if (!Surface)
	{
		fprintf(stderr, "Could not create surface.\n");
		return (-1);
	}
	fprintf(stderr, "Created surface.\n");

	// NOTE(Felix): Create Region
	/*struct wl_region *Region = wl_compositor_create_region(Compositor);
	if (!Region)
	{
		fprintf(stderr, "Could not create Region.\n");
		return (-1);
	}
	fprintf(stderr, "Created region.\n");
	wl_region_add(Region, 0, 0, RESOLUTION_WIDTH, RESOLUTION_HEIGHT);
	wl_surface_set_opaque_region(Surface, Region);
	wl_surface_set_input_region(Surface, Region);*/

	uint64_t Size = sizeof(uint32_t)*RESOLUTION_WIDTH*RESOLUTION_HEIGHT;
	uint64_t Stride = sizeof(uint32_t)*RESOLUTION_WIDTH;

	// NOTE(Felix): Memory to map the pool to
	char const *SharedMemoryName = "/MySharedMemory";
	int Fd = shm_open(SharedMemoryName, O_RDWR|O_CREAT, S_IRWXU);
	if (Fd == -1)
	{
		fprintf(stderr, "shm_open error.\n");
	}

	ftruncate(Fd, Size);
	void *Data = mmap(0, Size, PROT_READ|PROT_WRITE, MAP_SHARED, Fd, 0);
	if (Data == MAP_FAILED)
	{
		fprintf(stderr, "mmap failed.\n");
		return (-1);
	}
	
	// TODO(Felix): Get format with wl_shm_add_listener();
	struct wl_shm_pool *MemoryPool = wl_shm_create_pool(Shm,
														Fd,
														Size);

	if (!MemoryPool)
	{
		fprintf(stderr, "Unable to create shared memory pool.\n");
		return (-1);
	}
	fprintf(stderr, "Created shared memory pool.\n");

	
	// NOTE(Felix): Create wl_buffer
	struct wl_buffer *Buffer = wl_shm_pool_create_buffer(MemoryPool,
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

	
	// NOTE(Felix): We are currently missing wl_shell. But we are also supposed to replace that with xdg_shell,
	// but that documentation seems to be non-existant, even though the compositors are supposed to implement that.
	// what.
	struct xdg_surface *XdgSurface = xdg_wm_base_get_xdg_surface(Base, Surface);
	XdgToplevel = xdg_surface_get_toplevel(XdgSurface);

	xdg_surface_add_listener(XdgSurface, &xdg_surface_listener, 0);
	xdg_toplevel_add_listener(XdgToplevel, &xdg_toplevel_listener, 0);

	wl_surface_commit(Surface);
	wl_display_roundtrip(Display);
	

	// NOTE(Felix): Attach buffer to surface
	wl_surface_attach(Surface, Buffer,
					  0, 0);
	wl_surface_commit(Surface);

	while (wl_display_dispatch(Display) != -1 && GlobalRunning) 
	{
		static uint32_t Color = 0;
		for (uint32_t Y = 0; Y < RESOLUTION_HEIGHT; ++Y)
		{
			uint32_t *PixelY = Data + Y*Stride;
			for (uint32_t X = 0; X < RESOLUTION_WIDTH; ++X)
			{
				uint32_t *PixelX = PixelY+X;
				//*Pixel++ = 0xFF0000FF;
				*PixelX = Color++ % 0x000000FF;
			}
		}
	}

	// NOTE(Felix): Process events - look into "wl_event_queue"
	//munmap(Data, Size);
	//if (shm_unlink(SharedMemoryName) == -1)
	//{
	//	fprintf(stderr, "shm_unlink error.\n");
	//}
	xdg_toplevel_destroy(XdgToplevel);
	xdg_surface_destroy(XdgSurface);
	wl_surface_destroy(Surface);
	wl_display_disconnect(Display);
	return (0);
}
