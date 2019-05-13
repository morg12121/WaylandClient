#include <stdio.h>

#include <wayland-client.h>

int main(void)
{
	struct wl_display *Display = wl_display_connect(0);
	if (!Display) 
	{
		printf("Could not connect to Wayland display.\n");
		return (-1);
	}

	// NOTE(Felix): Process events - look into "wl_event_queue"
	

	wl_display_disconnect(Display);
	return (0);
}
