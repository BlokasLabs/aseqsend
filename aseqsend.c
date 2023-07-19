/*
 * aseqsend.c - send MIDI events to the ALSA sequencer clients
 *
 * Copyright (c) 2023 Vilniaus Blokas UAB <hello@blokas.io>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <alsa/asoundlib.h>
#include <stdio.h>

static snd_seq_t *g_seq;
static int g_port_count;
static snd_seq_addr_t *g_ports;

/* parses one or more port addresses from the string */
static void parse_ports(const char *arg)
{
	char *buf, *s, *port_name;
	int err;

	/* make a copy of the string because we're going to modify it */
	buf = strdup(arg);

	for (port_name = s = buf; s; port_name = s + 1) {
		/* Assume that ports are separated by commas.  We don't use
		 * spaces because those are valid in client names. */
		s = strchr(port_name, ',');
		if (s)
			*s = '\0';

		++g_port_count;
		g_ports = realloc(g_ports, g_port_count * sizeof(snd_seq_addr_t));

		err = snd_seq_parse_address(g_seq, &g_ports[g_port_count - 1], port_name);
		if (err < 0)
		{
			fprintf(stderr, "Invalid port %s - %s", port_name, snd_strerror(err));
			exit(EXIT_FAILURE);
		}
	}

	free(buf);
}

unsigned char parse_hex(const char *str)
{
	unsigned char b = 0;

	if (strlen(str) > 2)
		goto failure;

	const char *s = str;
	while (*s)
	{
		b <<= 4;
		if (*s >= '0' && *s <= '9')
			b |= *s - '0';
		else if (*s >= 'a' && *s <= 'f')
			b |= *s - 'a' + 10;
		else if (*s >= 'A' && *s <= 'F')
			b |= *s - 'A' + 10;
		else goto failure;
		++s;
	}

	return b;

failure:
	fprintf(stderr, "Expected hex byte, got '%s'\n", str);
	exit(EXIT_FAILURE);
}

static void print_usage()
{
	printf("Usage: aseqsend [--help] [--version] <client:port> 90 40 30 (hex bytes)\n");
}

static void print_version()
{
	printf("aseqsend 1.0.0, © Blokas https://blokas.io/\n");
}

static void parse_args(int argc, char **argv)
{
	int i;
	for (i=1; i<argc; ++i)
	{
		if (strncmp(argv[i], "--help", 7) == 0)
		{
			print_usage(argv[0]);
			printf("\n");
			print_version();
			exit(EXIT_SUCCESS);
		}
		else if (strncmp(argv[i], "--version", 10) == 0)
		{
			print_version();
			exit(EXIT_SUCCESS);
		}
	}

	if (argc < 3)
	{
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	parse_ports(argv[1]);
}

int main(int argc, char **argv)
{
	parse_args(argc, argv);

	int err = snd_seq_open(&g_seq, "default", SND_SEQ_OPEN_DUPLEX, 0);
	if (err < 0)
	{
		fprintf(stderr, "Error opening ALSA sequencer. (%d)\n", err);
		return 1;
	}

	err = snd_seq_set_client_name(g_seq, "aseqsend");
	if (err < 0)
	{
		fprintf(stderr, "Error setting client name. (%d)\n", err);
		return 1;
	}

	err = snd_seq_create_simple_port(g_seq, "aseqsend",
			SND_SEQ_PORT_CAP_WRITE,
			SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);

	if (err < 0)
	{
		fprintf(stderr, "Error creating sequencer port. (%d)\n", err);
		return 1;
	}

	snd_midi_event_t *midi_event;
	snd_midi_event_new(256, &midi_event);
	snd_seq_event_t ev;
	snd_seq_ev_clear(&ev);
	snd_seq_ev_set_subs(&ev);
	snd_seq_ev_set_direct(&ev);
	snd_seq_ev_set_source(&ev, 0);
	int i,j;
	for (i=2; i<argc; ++i)
	{
		unsigned char b = parse_hex(argv[i]);
		if (snd_midi_event_encode_byte(midi_event, b, &ev) == 1)
		{
			for (j=0; j<g_port_count; ++j)
			{
				snd_seq_ev_set_dest(&ev, g_ports[j].client, g_ports[j].port);
				snd_seq_event_output(g_seq, &ev);
			}
		}
	}
	snd_seq_drain_output(g_seq);
	snd_midi_event_free(midi_event);

	snd_seq_delete_simple_port(g_seq, 0);
	snd_seq_close(g_seq);

	return 0;
}
