#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <math.h>

#define BUFFADD 20
#define MEM_ERROR -1

typedef struct program
{
	char *name;
	int number_of_arguments;
	char **arguments;
	char *input_file, *output_file; //null if std
	int output_type; // 1 rewrite 2 append
} program;

typedef struct job
{
	int background;
	program* programs;
	int number_of_programs;
} job;

void err_com()
{
	printf("invalid input\n");
}

int read_command(char ***tokens)
{
	char **args = NULL;
	int numargs = 0;
	int memargs = 0;
	
	char* token = NULL;
	int len = 0;
	int mem = 0;
	char c;
	int quotes = 0;

	while (1)
	{
		c = fgetc(stdin);

		if ((!quotes && (c == ' ' || c == '#')) || c == '\n')
		{
			if (c == ' ' && len == 0)
				continue;

			if (len > 0)
			{
				token = (char*)realloc(token, len + 1);
				if (token == NULL) return MEM_ERROR;
				token[len] = '\0';

				if (++numargs > memargs)
				{
					args = (char**)realloc(args, sizeof(char*)*(memargs + BUFFADD));
					if (args == NULL) return MEM_ERROR;
					memargs += BUFFADD;
				}

				args[numargs - 1] = token;
			}

			if (c == '\n' || c == '#')
			{
				*tokens = args;
				return numargs;
			}

			token = NULL;
			len = 0;
			mem = 0;
		}
		else
		{
			if (c == '\"' || c == '\'')
				quotes = quotes ? 0 : 1;

			if (++len > mem)
			{
				token = (char*)realloc(token, mem + BUFFADD);
				if (token == NULL) return MEM_ERROR;
				mem += BUFFADD;
			}

			token[len - 1] = c;
		}
	}
}

int get_job(job *jb)
{
	int j;

	char** tokens = NULL;
	int numtokens;

	program *progs = NULL;
	int iprog = 0;

	char **args = NULL;
	int iarg = 0;
	int memarg = 0;

	jb->background = 0;

	progs = malloc(sizeof(program));
	if (progs == NULL) return MEM_ERROR;
	progs[0].input_file = NULL;
	progs[0].output_file = NULL;
	progs[0].output_type = 0;

	numtokens = read_command(&tokens);
	
	for (j = 0; j < numtokens; j++)
	{
		if (!strcmp(tokens[j], "|"))
		{
			args = (char**)realloc(args, sizeof(char*)*iarg);
			if (args == NULL) return MEM_ERROR;

			progs[iprog].arguments = args;
			progs[iprog].number_of_arguments = iarg - 1;

			iprog++;
			progs = (program*)realloc(progs, sizeof(program)*(iprog + 1));
			if (progs == NULL) return MEM_ERROR;
			progs[iprog].input_file = NULL;
			progs[iprog].output_file = NULL;
			progs[iprog].output_type = 0;

			args = NULL;
			iarg = 0;
			memarg = 0;
		}
		else if (!strcmp(tokens[j], "<"))
		{
			if (++j == numtokens)
			{
				err_com();
				return -1;
			}
			progs[iprog].input_file = tokens[j];
		}
		else if (!strcmp(tokens[j], ">") || !strcmp(tokens[j], ">>"))
		{
			if (++j == numtokens)
			{
				err_com();
				return -1;
			}
			progs[iprog].output_file = tokens[j];
			progs[iprog].output_type = !strcmp(tokens[j - 1], ">") ? 1 : 2;
		}
		else if (j == numtokens - 1 && !strcmp(tokens[j], "&"))
			jb->background = 1;
		else
		{
			if (iarg == 0)
				progs[iprog].name = tokens[j];
			else
			{
				if (memarg < iarg)
				{
					memarg += BUFFADD;
					args = (char**)realloc(args, sizeof(char*)*memarg);
					if (args == NULL) return MEM_ERROR;
				}
				args[iarg - 1] = tokens[j];
			}

			iarg++;
		}
	}
	args = (char**)realloc(args, sizeof(char*)*iarg);
	if (args == NULL) return MEM_ERROR;

	progs[iprog].arguments = args;
	progs[iprog].number_of_arguments = iarg - 1;

	jb->programs = progs;
	jb->number_of_programs = iprog + 1;


	return 0;
}

int main()
{
	int i;
	int j;

	job jb;

	get_job(&jb);

	for (i = 0; i < jb.number_of_programs; i++)
	{
		printf("program %d:\n\tname : %s\n\tnum of agrs : %d", i + 1, jb.programs[i].name, jb.programs[i].number_of_arguments);
		for (j = 0; j < jb.programs[i].number_of_arguments; j++)
			printf("\n\targument %d : %s", j + 1, jb.programs[i].arguments[j]);
		printf("\n\tinput file : %s\n\toutput file : %s\n\toutput type : %d\n", jb.programs[i].input_file == NULL ? "NULL" : jb.programs[i].input_file,
			jb.programs[i].output_file == NULL ? "NULL" : jb.programs[i].output_file, jb.programs[i].output_type);
	}
	printf("background : %d\n", jb.background);
}
