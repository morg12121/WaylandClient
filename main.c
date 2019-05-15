#include <stdio.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <wayland-client.h>

#define RESOLUTION_WIDTH 1280
#define RESOLUTION_HEIGHT 720

struct wl_display *Display = 0;
struct wl_compositor *Compositor = 0;
struct wl_shm *Shm = 0;

static void
global_registry_handler(void *Data, struct wl_registry *Registry,
						uint32_t Id,
						const char *Interface, uint32_t Version)
{
	printf("Registry event for %s with ID %d received.\n", Interface, Id);
	// NOTE(Felix): Retreive global objects
	if (strcmp(Interface, "wl_compositor") == 0)
	{
		Compositor = wl_registry_bind(Registry,
									  Id,
									  &wl_compositor_interface,
									  1);
	}
	if (strcmp(Interface, "wl_shm") == 0)
	{
		Shm = wl_registry_bind(Registry,
		                       Id,
		                       &wl_shm_interface,
		                       1);
	}
}

static void
global_registry_remover(void *Data, struct wl_registry *Registry,
						uint32_t Id)
{
	printf("Registry remove call received.\n");
}

static const struct wl_registry_listener GlobalRegistryListener = {
	global_registry_handler,
	global_registry_remover
};

int 
main(void)
{
	Display = wl_display_connect(0);
	if (!Display) 
	{
		printf("Could not connect to Wayland display.\n");
		return (-1);
	}

	struct wl_registry *Registry = wl_display_get_registry(Display);
	wl_registry_add_listener(Registry, &GlobalRegistryListener, 0);
	wl_display_dispatch(Display);
	wl_display_roundtrip(Display);

	if (!Compositor)
	{
		printf("Cannot find compositor.\n");
		return (-1);
	}
	printf("Found compositor.\n");

	if (!Shm)
	{
		printf("Cannot find global shared memory object.\n");
		return (-1);
	}
	printf("Found global shared memory object.\n");

	// NOTE(Felix): Create surface - is used to draw stuff on it
	struct wl_surface *Surface = wl_compositor_create_surface(Compositor);
	if (!Surface)
	{
		printf("Could not create surface.\n");
		return (-1);
	}
	printf("Created surface.\n");

	// NOTE(Felix): Create Region
	struct wl_region *Region = wl_compositor_create_region(Compositor);
	if (!Region)
	{
		printf("Could not create Region.\n");
		return (-1);
	}
	printf("Created region.\n");
	wl_region_add(Region, 0, 0, RESOLUTION_WIDTH, RESOLUTION_HEIGHT);
	wl_surface_set_opaque_region(Surface, Region);
	wl_surface_set_input_region(Surface, Region);

	// NOTE(Felix): Memory to map the pool to
	char const *SharedMemoryName = "/MySharedMemory";
	int Fd = shm_open(SharedMemoryName, O_RDWR|O_CREAT, S_IRWXU);
	
	// TODO(Felix): Get format with wl_shm_add_listener();
	struct wl_shm_pool *MemoryPool = wl_shm_create_pool(Shm,
														Fd,
														sizeof(uint32_t)*RESOLUTION_WIDTH*RESOLUTION_HEIGHT);

	if (!MemoryPool)
	{
		printf("Unable to create shared memory pool.\n");
		return (-1);
	}
	printf("Created shared memory pool.\n");

	
	// NOTE(Felix): Create wl_buffer
	struct wl_buffer *Buffer = wl_shm_pool_create_buffer(MemoryPool,
														 0,
														 RESOLUTION_WIDTH,
														 RESOLUTION_HEIGHT,
														 sizeof(uint32_t)*RESOLUTION_WIDTH,
														 WL_SHM_FORMAT_RGBA8888);

	if (!Buffer)
	{
		printf("Unable to create buffer from shared memory.\n");
		return (-1);
	}
	printf("Created buffer");

	
	// NOTE(Felix): Attach buffer to surface
	wl_surface_attach(Surface, Buffer,
					  0, 0);
	wl_surface_commit(Surface);
		
	// NOTE(Felix): We are currently missing wl_shell. But we are also supposed to replace that with xdg_shell,
	// but that documentation seems to be non-existant, even though the compositors are supposed to implement that.
	// what.
	
	//struct xdg_surface *XdgSurface = xdg_shell_get_xdg_surface(xdg_shell, Surface);
	//struct xdg_surface *XdgSurface = xdg_wm_base_get_xdg_surface(xdg_wm_base, surface);
	
	

	// NOTE(Felix): Process events - look into "wl_event_queue"
	

	wl_display_disconnect(Display);
	return (0);
}
