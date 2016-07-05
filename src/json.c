/**
 * json -- JSON love for your command line - AND IT'S ALL IN C, BABY!
 * Copyright (c) 2016 Alex Caudill. All rights reserved.
 *
 * Derived from <https://github.com/trentm/json> and <https://trentm.com/json/>
 * Copyright (c) 2014 Trent Mick. All rights reserved.
 * Copyright (c) 2014 Joyent Inc. All rights reserved.
 */

#define _X_OPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include "duktape.h"

/* Accepted flags for getopt_long */
#define OPTSTRING "aAc:d:D:e:f:gknIijo:qv024h?"
/* Output modes */
#define OM_JSONY   1
#define OM_JSON    2
#define OM_INSPECT 3
#define OM_COMPACT 4
/* Default to 'jsony' output. */
static int output_mode = OM_JSONY;
/* Flag set by ‘--array’. */
static int array_flag = 0;
/* Flag set by ‘--drop-hdr’. */
static int drop_hdr_flag = 0;
/* Flag set by ‘--file’. */
static int file_flag = 0;
/* Flag set by ‘--group’. */
static int group_flag = 0;
/* Flag set by ‘--merge’. */
static int merge_flag = 0;
/* Flag set by ‘--deep-merge’. */
static int deepmerge_flag = 0;
/* Flag set by ‘--quiet’. */
static int quiet_flag = 0;
/* Flag set by ‘--validate’. */
static int validate_flag = 0;

static void shortHelp() {
	printf("Invalid usage. Try -h or -? for help.\n");
}

static void longHelp() {
	printf(
		"Usage:\n"
		"<something generating JSON on stdout> | json [OPTIONS] [LOOKUPS...]\n"
		"  json -f FILE [OPTIONS] [LOOKUPS...]\n"
		"\n"
		"Pipe in your JSON for pretty-printing, JSON validation, filtering,\n"
		"and modification. Supply one or more `LOOKUPS` to extract a\n"
		"subset of the JSON. HTTP header blocks are skipped by default.\n"
		"Roughly in order of processing, features are:\n"
		"\n"
		"Grouping:\n"
		"  Use '-g' or '--group' to group adjacent objects, separated by"
		"  by no space or a by a newline, or adjacent arrays, separate by"
		"  by a newline. This can be helpful for, e.g.: \n"
		"     $ cat *.json | json -g ... \n"
		"  and similar.\n"
		"\n"
		"Execution:\n"
		"  Use the '-e CODE' option to execute JavaScript code on the input JSON.\n"
		"     $ echo \'{'name':'trent','age':38}\' | json -e \'this.age++\'\n"
		"     {\n"
		"       'name': 'trent',\n"
		"       'age': 39\n"
		"     }\n"
		"  If input is an array, this will automatically process each\n"
		"  item separately.\n"
		"\n"
		"Conditional filtering:\n"
		"  Use the '-c CODE' option to filter the input JSON.\n"
		"     $ echo \'[{'age':38},{'age':4}]\' | json -c \'this.age>21\'\n"
		"     [{\'age\':38}]\n"
		"  If input is an array, this will automatically process each\n"
		"  item separately. Note: 'CODE' is JavaScript code.\n"
		"\n"
		"Lookups:\n"
		"  Use lookup arguments to extract particular values:\n"
		"     $ echo \'{'name':'trent','age':38}\' | json name\n"
		"     trent\n"
		"\n"
		"  Use '-a' for *array processing* of lookups and *tabular output*:\n"
		"     $ echo \'{'name':'trent','age':38}\' | json name age\n"
		"     trent\n"
		"     38\n"
		"     $ echo \'[{'name':'trent','age':38},\n"
		"               {'name':'ewan','age':4}]\' | json -a name age\n"
		"     trent 38\n"
		"     ewan 4\n"
		"\n"
		"In-place editing:\n"
		"  Use '-I, --in-place' to edit a file in place:\n"
		"     $ json -I -f config.json  # reformat\n"
		"     $ json -I -f config.json -c \'this.logLevel=\"debug\"\' # add field\n"
		"\n"
		"Pretty-printing:\n"
		"  Output is 'jsony' by default: 2-space indented JSON, except a\n"
		"  single string value is printed without quotes.\n"
		"     $ echo \'{\"name\": \"trent\", \"age\": 38}\' | json\n"
		"     {\n"
		"       \"name\": \"trent\",\n"
		"       \"age\": 38\n"
		"     }\n"
		"     $ echo \'{\"name\": \"trent\", \"age\": 38}\' | json name\n"
		"     trent\n"
		"\n"
		"  Use '-j' or '-o json' for explicit JSON, '-o json-N' for N-space indent:\n"
		"'     $ echo \'{\"name\": \"trent\", \"age\": 38}\' | json -o json-0\n"
		"'     {\"name\":\"trent\",\"age\":38}\n"
		"\n"
		"Options:\n"
		"  -h, --help    Print this help info and exit.\n"
		"  -v, --version     Print version of this command and exit.\n"
		"  -q, --quiet   Don\'t warn if input isn\'t valid JSON.\n"
		"\n"
		"  -f FILE       Path to a file to process. If not given, then\n"
		"                stdin is used."
		"  -I, --in-place  In-place edit of the file given with \"-f\".\n"
		"                Lookups are not allowed with in-place editing\n"
		"                because it makes it too easy to lose content.\n"
		"\n"
		"  -H, --drop-hdr Drop any HTTP header block (as from `curl -i ...`).\n"
		"  -g, --group   Group adjacent objects or arrays into an array.\n"
		"  -m --merge    Merge adjacent objects into one. Keys in last \n"
		"                object win.\n"
		"  -M, --deep-merge Same as '--merge', but will recurse into objects \n"
		"                under the same key in both.\n"
		"  -a, --array   Process input as an array of separate inputs\n"
		"                and output in tabular form.\n"
		"  -A            Process input as a single object, i.e. stop\n"
		"                \"-e\" and \"-c\" automatically processing each\n"
		"                item of an input array.\n"
		"  -d DELIM      Delimiter char for tabular output (default is \" \").\n"
		"  -D DELIM      Delimiter char between lookups (default is \".\"). E.g.:\n"
		"                    $ echo \'{\"a.b\": {\"b\": 1}}\' | json -D / a.b/b\n"
		"\n"
		"  -M, --items   Itemize an object into an array of \n"
		"                    {\"key\": <key>, \"value\": <value>}\n"
		"                objects for easier processing.\n"
		"\n"
		"  -e CODE       Execute the given JavaScript code on the input. If input\n"
		"                is an array, then each item of the array is processed\n"
		"                separately (use '-A' to override).\n"
		"  -c CODE       Filter the input with JavaScript `CODE`. If `CODE`\n"
		"                returns false-y, then the item is filtered out. If\n"
		"                input is an array, then each item of the array is \n"
		"                processed separately (use '-A' to override).\n"
		"\n"
		"  -k, --keys    Output the input object\'s keys.\n"
		"  -n, --validate  Just validate the input (no processing or output).\n"
		"                Use with \"-q\" for silent validation (exit status).\n"
		"\n"
		"  -o, --output MODE\n"
		"                Specify an output mode. One of:\n"
		"                    jsony (default): JSON with string quotes elided\n"
		"                    json: JSON output, 2-space indent\n"
		"                    json-N: JSON output, N-space indent, e.g. 'json-4'\n"
		"                    inspect: node.js `util.inspect` style output\n"
		"  -i            Shortcut for `-o inspect`\n"
		"  -j            Shortcut for `-o json`\n"
		"  -0, -2, -4    Set indentation to the given value w/o setting MODE.\n"
		"                    -0   =>  -o jsony-0\n"
		"                    -4   =>  -o jsony-4\n"
		"                    -j0  =>  -o json-0\n"
		"\n"
		"See <http://trentm.com/json> and <https://github.com/trentm/json> for \n"
		"project details on the original pure Javascript version of this command.\n"
		"\n"
		"See <https://github.com/hypoalex/json> for info on this pure C json(1).\n"
	);
}

/**
 * Return a *shallow* copy of the given JSON object.
 *
 * Only support objects that you get out of JSON, i.e. no functions.
 */
//json_t *
//objCopy(json_t *obj) {
	
//}

int
main (int argc, char **argv)
{
	FILE *input;  /* defaults to stdin  */
	FILE *output; /* defaults to stdout */
	
	int c; /* Used by getopt_long(). */

	/* If invoked without any arguments, print the short help and exit non-cleanly. */
	if (argc == 1) {
		shortHelp();
		exit(1);
	}

	static struct option long_options[] =
	{
		/* These options require no argument and set a flag. */
		{"help",       no_argument, 0, 'h'},
		{"array",      no_argument, &array_flag, 'a'},
		{"drop-hdr",   no_argument, &drop_hdr_flag, 'H'},
		{"group",      no_argument, &group_flag, 'g'},
		{"merge",      no_argument, &merge_flag,     'm'},
		{"deep-merge", no_argument, &deepmerge_flag, 'M'},
		{"validate",   no_argument, &validate_flag,  'n'},
		/* These options require an argument and set a flag. */
		{"file",       required_argument, &file_flag, 'f'},
		/* ...and these options don’t set a flag. */
		{"append",     no_argument,       0, 'b'},
		{"delim",      required_argument, 0, 'd'},
		{"create",     required_argument, 0, 'c'},
		{"in-place",   required_argument, 0, 'I'},
		{"quiet",      no_argument,       0, 'q'},
		{"help",       no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};

	while (1) 
	{
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long (argc, argv, OPTSTRING, long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c)
		{
			case 0:
			/* If this option set a flag, do nothing else for now. */
				if (long_options[option_index].flag != 0)
					break;
				printf ("option %s", long_options[option_index].name);
				if (optarg)
					printf (" with arg %s", optarg);
				printf ("\n");
				break;

			case 'a':
				puts ("option -a\n");
				break;

			case 'c':
				printf ("option -c with value `%s'\n", optarg);
				break;

			case 'd':
				printf ("option -d with value `%s'\n", optarg);
				break;
			case 'D':
				printf ("option -D with value `%s'\n", optarg);
				break;
			case 'f':
				printf ("option -f with value `%s'\n", optarg);
				break;
			case 'g':
				printf ("option -g with value `%s'\n", optarg);
				break;
			case 'I':
				break;
			case 'n':
				validate_flag = 1;
				break;
			case '0':
			case '2':
			case '4':
				printf("nlaj\n");
				break;
			case 'h':
			case '?':
			default:
				longHelp();
				exit(0);
		}
	}

	if (validate_flag)
		puts ("validate flag is set!\n");

	/* The remaining command line arguments (not options) are lookups. */
	if (optind < argc) {
		printf ("non-option ARGV-elements: ");
		while (optind < argc)
			printf ("%s ", argv[optind++]);
		putchar ('\n');
	}
	
	/* First, we'll load our input. */
	if (file_flag) { // Read from the specified files.
	
	} else { // Read from stdin.
		input = stdin;
	}
	
	/* 
	 * Nearly all of the actual work is done here with a duktape context to handle
	 * parsing/writing the JSON and executing the JavaScript input. This is a sort
	 * of minimalist equivalent to V8, allowing this program to be self-contained.
 	 */
        duk_context *ctx = duk_create_heap_default();
	if (ctx) {
		duk_eval_string(ctx, "print('Hello world!');");		
	
		duk_push_string(ctx, "{\"meaningOfLife\":42}");
		duk_json_decode(ctx, -1);
		duk_get_prop_string(ctx, -1, "meaningOfLife");
		printf("JSON decoded meaningOfLife is: %s\n", duk_to_string(ctx, -1));
		duk_pop_2(ctx);
		
		duk_destroy_heap(ctx);
	} else {
		printf("An unknown error occurred!\n");
		printf("Please file a github issue at https://github.com/hypoalex/json");
	}

	exit (0);
}
