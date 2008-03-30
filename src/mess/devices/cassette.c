/*********************************************************************

	cassette.c

	MESS interface to the cassette image abstraction code

*********************************************************************/

#include "mame.h"
#include "cassette.h"
#include "formats/cassimg.h"
#include "ui.h"


#define CASSETTE_TAG		"cassette"
#define ANIMATION_FPS		4
#define ANIMATION_FRAMES	4

#define VERBOSE				0
#define LOG(x) do { if (VERBOSE) logerror x; } while (0)


/* from devices/mflopimg.c */
extern struct io_procs mess_ioprocs;


struct mess_cassetteimg
{
	cassette_image *cassette;
	cassette_state state;
	double position;
	double position_time;
	INT32 value;
};


static struct mess_cassetteimg *get_cassimg(const device_config *image)
{
	return image_lookuptag(image, CASSETTE_TAG);
}



static cassette_state get_default_state(const struct IODevice *dev)
{
	assert(dev->type == IO_CASSETTE);
	return (cassette_state) (int) mess_device_get_info_int(&dev->devclass, MESS_DEVINFO_INT_CASSETTE_DEFAULT_STATE);
}



/*********************************************************************
	cassette IO
*********************************************************************/

static int cassette_is_motor_on(const device_config *cassette)
{
	cassette_state state;
	state = cassette_get_state(cassette);
	if ((state & CASSETTE_MASK_UISTATE) == CASSETTE_STOPPED)
		return FALSE;
	if ((state & CASSETTE_MASK_MOTOR) != CASSETTE_MOTOR_ENABLED)
		return FALSE;
	return TRUE;
}



static void cassette_update(const device_config *cassette)
{
	struct mess_cassetteimg *tag;
	double cur_time;
	double new_position;

	cur_time = attotime_to_double(timer_get_time());
	tag = get_cassimg(cassette);

	if (cassette_is_motor_on(cassette) && tag->cassette)
	{
		new_position = tag->position + (cur_time - tag->position_time);

		switch(tag->state & CASSETTE_MASK_UISTATE) {
		case CASSETTE_RECORD:
			cassette_put_sample(tag->cassette, 0, tag->position, new_position - tag->position, tag->value);
			break;

		case CASSETTE_PLAY:
			cassette_get_sample(tag->cassette, 0, new_position, 0.0, &tag->value);
			break;
		}
		tag->position = new_position;
	}
	tag->position_time = cur_time;
}



cassette_state cassette_get_state(const device_config *cassette)
{
	return get_cassimg(cassette)->state;
}



void cassette_change_state(const device_config *cassette, cassette_state state, cassette_state mask)
{
	struct mess_cassetteimg *tag;
	cassette_state new_state;

	tag = get_cassimg(cassette);
	new_state = tag->state;
	new_state &= ~mask;
	new_state |= (state & mask);

	if (new_state != tag->state)
	{
		cassette_update(cassette);
		tag->state = new_state;
	}
}



void cassette_set_state(const device_config *cassette, cassette_state state)
{
	cassette_change_state(cassette, state, ~0);
}



double cassette_input(const device_config *cassette)
{
	INT32 sample;
	double double_value;
	struct mess_cassetteimg *tag;

	cassette_update(cassette);
	tag = get_cassimg(cassette);
	sample = tag->value;
	double_value = sample / ((double) 0x7FFFFFFF);

	LOG(("cassette_input(): time_index=%g value=%g\n", tag->position, double_value));

	return double_value;
}



void cassette_output(const device_config *cassette, double value)
{
	struct mess_cassetteimg *tag;
	tag = get_cassimg(cassette);
	if (((tag->state & CASSETTE_MASK_UISTATE) == CASSETTE_RECORD) && (tag->value != value))
	{
		cassette_update(cassette);

		value = MIN(value, 1.0);
		value = MAX(value, -1.0);

		tag->value = (INT32) (value * 0x7FFFFFFF);
	}
}



cassette_image *cassette_get_image(const device_config *cassette)
{
	struct mess_cassetteimg *tag;
	tag = get_cassimg(cassette);
	return tag->cassette;
}



double cassette_get_position(const device_config *cassette)
{
	double position;
	struct mess_cassetteimg *tag;

	tag = get_cassimg(cassette);
	position = tag->position;

	if (cassette_is_motor_on(cassette))
		position += attotime_to_double(timer_get_time()) - tag->position_time;
	return position;
}



double cassette_get_length(const device_config *cassette)
{
	struct mess_cassetteimg *tag;
	struct CassetteInfo info;

	tag = get_cassimg(cassette);
	cassette_get_info(tag->cassette, &info);
	return ((double) info.sample_count) / info.sample_frequency;
}



void cassette_seek(const device_config *cassette, double time, int origin)
{
	struct mess_cassetteimg *tag;

	cassette_update(cassette);

	switch(origin) {
	case SEEK_SET:
		break;

	case SEEK_END:
		time += cassette_get_length(cassette);
		break;

	case SEEK_CUR:
		time += cassette_get_position(cassette);
		break;
	}

	/* clip position into legal bounds */
	tag = get_cassimg(cassette);
	tag->position = time;
}



/*********************************************************************
	cassette device init/load/unload/specify
*********************************************************************/

static DEVICE_START( cassette )
{
	const struct IODevice *dev;

	image_alloctag(device, CASSETTE_TAG, sizeof(struct mess_cassetteimg));

	/* set to default state */
	dev = mess_device_from_core_device(device);
	get_cassimg(device)->state = get_default_state(dev);
}



static DEVICE_LOAD( cassette )
{
	casserr_t err;
	int cassette_flags;
	struct mess_cassetteimg *tag;
	const struct IODevice *dev;
	const struct CassetteFormat **formats;
	const struct CassetteOptions *create_opts;
	const char *extension;
	int is_writable;

	tag = get_cassimg(image);

	/* figure out the cassette format */
	dev = device_find_from_machine(image->machine, IO_CASSETTE);
	formats = mess_device_get_info_ptr(&dev->devclass, MESS_DEVINFO_PTR_CASSETTE_FORMATS);

	if (image_has_been_created(image))
	{
		/* creating an image */
		create_opts = (const struct CassetteOptions *) mess_device_get_info_ptr(&dev->devclass, MESS_DEVINFO_PTR_CASSETTE_OPTIONS);
		err = cassette_create((void *) image, &mess_ioprocs, &wavfile_format, create_opts, CASSETTE_FLAG_READWRITE|CASSETTE_FLAG_SAVEONEXIT, &tag->cassette);
		if (err)
			goto error;
	}
	else
	{
		/* opening an image */
		do
		{
			is_writable = image_is_writable(image);
			cassette_flags = is_writable ? (CASSETTE_FLAG_READWRITE|CASSETTE_FLAG_SAVEONEXIT) : CASSETTE_FLAG_READONLY;
			extension = image_filetype(image);
			err = cassette_open_choices((void *) image, &mess_ioprocs, extension, formats, cassette_flags, &tag->cassette);

			/* this is kind of a hack */
			if (err && is_writable)
				image_make_readonly(image);
		}
		while(err && is_writable);

		if (err)
			goto error;
	}

	/* set to default state, but only change the UI state */
	cassette_change_state(image, get_default_state(dev), CASSETTE_MASK_UISTATE);

	/* reset the position */
	tag->position = 0.0;
	tag->position_time = attotime_to_double(timer_get_time());

	return INIT_PASS;

error:
	return INIT_FAIL;
}



static DEVICE_UNLOAD( cassette )
{
	struct mess_cassetteimg *tag;

	tag = get_cassimg(image);

	/* if we are recording, write the value to the image */
	if ((tag->state & CASSETTE_MASK_UISTATE) == CASSETTE_RECORD)
		cassette_update(image);

	/* close out the cassette */
	cassette_close(tag->cassette);
	tag->cassette = NULL;

	/* set to default state, but only change the UI state */
	cassette_change_state(image, CASSETTE_STOPPED, CASSETTE_MASK_UISTATE);
}



/*
	display a small tape icon, with the current position in the tape image
*/
static void device_display_cassette(const device_config *image)
{
	char buf[65];
	float x, y;
	int n;
	double position, length;
	cassette_state uistate;

	/* abort if we should not be showing the image */
	if (!image_exists(image))
		return;
	if (!cassette_is_motor_on(image))
		return;

	/* figure out where we are in the cassette */
	position = cassette_get_position(image);
	length = cassette_get_length(image);
	uistate = cassette_get_state(image) & CASSETTE_MASK_UISTATE;

	/* choose a location on the screen */
	x = 0.0f;
	y = image_index_in_device(image) * ui_get_line_height();

	/* choose which frame of the animation we are at */
	n = ((int) position / ANIMATION_FPS) % ANIMATION_FRAMES;

	/* character pairs 2-3, 4-5, 6-7, 8-9 form little tape cassette images */
	snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%c%c %c %02d:%02d (%04d) [%02d:%02d (%04d)]",
		n * 2 + 2,								/* cassette icon left */
		n * 2 + 3,								/* cassette icon right */
		(uistate == CASSETTE_PLAY) ? 16 : 14,	/* play or record icon */
		((int) position / 60),
		((int) position % 60),
		(int) position,
		((int) length / 60),
		((int) length % 60),
		(int) length);

	/* draw the cassette */
	ui_draw_text_box(buf, JUSTIFY_LEFT, x, y, UI_FILLCOLOR);
}



void cassette_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	char *s;
	int i;
	const struct CassetteFormat **formats;

	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_TYPE:						info->i = IO_CASSETTE; break;
		case MESS_DEVINFO_INT_READABLE:					info->i = 1; break;
		case MESS_DEVINFO_INT_WRITEABLE:					info->i = 1; break;
		case MESS_DEVINFO_INT_CREATABLE:					info->i = 1; break;
		case MESS_DEVINFO_INT_CASSETTE_DEFAULT_STATE:	info->i = CASSETTE_PLAY; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_INIT:						info->init = DEVICE_START_NAME(cassette); break;
		case MESS_DEVINFO_PTR_LOAD:						info->load = device_load_cassette; break;
		case MESS_DEVINFO_PTR_UNLOAD:					info->unload = device_unload_cassette; break;
		case MESS_DEVINFO_PTR_DISPLAY:					info->display = device_display_cassette; break;
		case MESS_DEVINFO_PTR_CASSETTE_FORMATS:			info->p = (void *) cassette_default_formats; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_DEV_FILE:					strcpy(info->s = device_temp_str(), __FILE__); break;
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:
			formats = mess_device_get_info_ptr(devclass, MESS_DEVINFO_PTR_CASSETTE_FORMATS);

			/* set up a temporary string */
			s = device_temp_str();
			info->s = s;
			s[0] = '\0';

			/* append each of the extensions */
			for (i = 0; formats[i]; i++)
				specify_extension(s, 256, formats[i]->extensions);
			break;
	}
}
