#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#ifndef CONSUMER
#define CONSUMER "Consumer"
#endif

#define N_LED 4
#define N_BUTTON 3
#define MAX_TIME 15
#define START_STOP_PAUSE 0
#define DEC_TIME 1
#define INC_TIME 2
#define BOUNCING 3
#define NANO 1000000000
#define RISING 1
#define FALLING 2

int led_lines[N_LED] = {24, 25, 26, 27};
int button_lines[N_BUTTON] = {12, 13, 14};

int display_time(struct gpiod_line_bulk led_bulk, int time);

int main(int argc, char **argv)
{
	char *chipname = "gpiochip1";
	struct timespec ts = {0, NANO / BOUNCING};
	struct gpiod_line_event event;
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	int ret;
	double time;
	bool active, pause, pressed, ssp_pressed;
	struct gpiod_line_bulk led_bulk, button_bulk, event_bulk;

	chip = gpiod_chip_open_by_name(chipname);
	if (!chip)
	{
		perror("Open chip failed\n");
		ret = -1;
		goto end;
	}

	//LEDs
	ret = gpiod_chip_get_lines(chip, led_lines, N_LED, &led_bulk);
	if (ret < 0) {
		perror("Get lines failed\n");
		ret = -1;
		goto close_chip;
	}
	for (int i = 0; i < led_bulk.num_lines; ++i)
	{
		ret = gpiod_line_request_output(led_bulk.lines[i], CONSUMER, 0);
		if (ret < 0)
		{
			perror("Request line as output failed\n");
			ret = -1;
			goto led_release_line;
		}
	}

	//buttons
	ret = gpiod_chip_get_lines(chip, button_lines, N_BUTTON, &button_bulk);
	if (ret < 0)
	{
		perror("Get button lines failed\n");
		ret = -1;
		goto close_chip;
	}
	ret = gpiod_line_request_bulk_both_edges_events(&button_bulk, CONSUMER);
	if (ret < 0)
	{
		perror("Request event notification failed\n");
		ret = -1;
		goto release_line;
	}

	time = 0;
	active = false;
	pause = false;

	while (true)
	{
		while (true)
		{
			ret = gpiod_line_event_wait_bulk(&button_bulk, &ts, &event_bulk);
			if (ret < 0)
			{
				perror("Wait event notification failed\n");
				ret = -1;
				goto release_line;
			}
			else if (ret == 0)
			{
				if (pressed)
				{
					if(line == button_bulk.lines[START_STOP_PAUSE])
					{
						if (event.event_type == FALLING)
						{
							ssp_pressed = true;
						}
						else if (ssp_pressed)
						{
							ssp_pressed = false;
						}
						else if (!active)
						{
							active = true;
						}
					}
					else if (line == button_bulk.lines[DEC_TIME])
					{
						if (time >= 1)
						{
							time--;
						}
					}
					else if (line == button_bulk.lines[INC_TIME])
					{
						if (time <= MAX_TIME - 1)
						{
							time++;
						}
					}
					if (active)
					{
						break;
					}
				}
				pressed = false;
				ret = display_time(led_bulk, (int)time);
				if (ret < 0)
				{
					perror("Set line output failed\n");
					return ret;
				}
				continue;
			}

			line = event_bulk.lines[0];
			ret = gpiod_line_event_read(line, &event);
			pressed = true;
			if (ret < 0)
			{
				perror("Read last event notification failed\n");
				ret = -1;
				goto release_line;
			}
		}
		pressed = false;
		while (true)
		{
			ret = gpiod_line_event_wait(button_bulk.lines[START_STOP_PAUSE], &ts);
			if (ret < 0)
			{
				perror("Wait event notification failed\n");
				ret = -1;
				goto release_line;
			}
			else if (ret == 0)
			{
				if (pressed)
				{
					if (event.event_type == FALLING)
					{
						ssp_pressed = true;
					}
					else if (ssp_pressed)
					{
						active = false;
						time = 0;
					}
					else
					{
						pause = !pause;
					}

					if (!active)
					{
						pause = false;
						break;
					}
				}
				pressed = false;
				if(pause)
				{
					continue;
				}
				time -= (double)(ts.tv_nsec) / NANO;
				if(time < 0)
				{
					time = 0;
					active = false;
					pause = false;
					break;
				}
				ret = display_time(led_bulk, (int)time);
				if (ret < 0)
				{
					perror("Set line output failed\n");
					ret = -1;
					goto release_line;
				}
				continue;
			}

			line = event_bulk.lines[0];
			ret = gpiod_line_event_read(line, &event);
			pressed = true;
			if (ret < 0)
			{
				perror("Read last event notification failed\n");
				ret = -1;
				goto release_line;
			}
		}
	}

	ret = 0;

release_line:
	gpiod_line_release_bulk(&button_bulk);
led_release_line:
	gpiod_line_release_bulk(&led_bulk);
close_chip:
	gpiod_chip_close(chip);
end:
	return ret;
}

int display_time(struct gpiod_line_bulk led_bulk, int time)
{
	int ret = 0;
	if (time < 0 || time >= N_LED * N_LED)
	{
		return -1;
	}
	for (int i = 0; i < led_bulk.num_lines; ++i)
	{
		ret = gpiod_line_set_value(led_bulk.lines[N_LED - i - 1], (time >> i) & 1);
		if (ret < 0)
		{
			return ret;
		}
	}
	return ret;
}