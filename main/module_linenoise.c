#include "sdkconfig.h"
//#if defined(CONFIG_LINENOISE_ENABLED)

#include <freertos/FreeRTOSConfig.h>
#include <esp_task.h>
#include <duktape.h>
#include "duktape_utils.h"
#include <esp_log.h>
#include "esp_system.h"
#include "esp_console.h"
#include "driver/uart.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"

#include "logging.h"
#include "module_linenoise.h"

LOG_TAG("module_linenoise");

/* linenoise.c -- guerrilla line editing library against the idea that a
 * line editing lib needs to be 20,000 lines of C code.
 *
 * You can find the latest source code at:
 *
 *   http://github.com/antirez/linenoise
 *
 * Does a number of crazy assumptions that happen to be true in 99.9999% of
 * the 2010 UNIX computers around.
 *
 * ------------------------------------------------------------------------
 *
 * Copyright (c) 2010-2016, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2010-2013, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *	 notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *	 notice, this list of conditions and the following disclaimer in the
 *	 documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ------------------------------------------------------------------------
 *
 * References:
 * - http://invisible-island.net/xterm/ctlseqs/ctlseqs.html
 * - http://www.3waylabs.com/nw/WWW/products/wizcon/vt220.html
 *
 * Todo list:
 * - Filter bogus Ctrl+<char> combinations.
 * - Win32 support
 *
 * Bloat:
 * - History search like Ctrl+r in readline?
 *
 * List of escape sequences used by this program, we do everything just
 * with three sequences. In order to be so cheap we may have some
 * flickering effect with some slow terminal, but the lesser sequences
 * the more compatible.
 *
 * EL (Erase Line)
 *	Sequence: ESC [ n K
 *	Effect: if n is 0 or missing, clear from cursor to end of line
 *	Effect: if n is 1, clear from beginning of line to cursor
 *	Effect: if n is 2, clear entire line
 *
 * CUF (CUrsor Forward)
 *	Sequence: ESC [ n C
 *	Effect: moves cursor forward n chars
 *
 * CUB (CUrsor Backward)
 *	Sequence: ESC [ n D
 *	Effect: moves cursor backward n chars
 *
 * The following is used to get the terminal width if getting
 * the width with the TIOCGWINSZ ioctl fails
 *
 * DSR (Device Status Report)
 *	Sequence: ESC [ 6 n
 *	Effect: reports the current cusor position as ESC [ n ; m R
 *			where n is the row and m is the column
 *
 * When multi line mode is enabled, we also use an additional escape
 * sequence. However multi line editing is disabled by default.
 *
 * CUU (Cursor Up)
 *	Sequence: ESC [ n A
 *	Effect: moves cursor up of n chars.
 *
 * CUD (Cursor Down)
 *	Sequence: ESC [ n B
 *	Effect: moves cursor down of n chars.
 *
 * When linenoiseClearScreen() is called, two additional escape sequences
 * are used in order to clear the screen and position the cursor at home
 * position.
 *
 * CUP (Cursor position)
 *	Sequence: ESC [ H
 *	Effect: moves the cursor to upper left corner
 *
 * ED (Erase display)
 *	Sequence: ESC [ 2 J
 *	Effect: clear the whole screen
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <unistd.h>


/* All the following were not static, so js front ends might be worthwhile */

static int linenoiseHistoryAdd(const char *line);
static int linenoiseHistorySetMaxLen(int len);
//static void linenoiseHistoryFree();
static void linenoiseClearScreen(void);
static void linenoiseSetMultiLine(int ml);

#define LINENOISE_DEFAULT_HISTORY_MAX_LEN 100
#define LINENOISE_MAX_LINE 256
#define LINENOISE_MAX_PROMPT 32
static int mlmode = 0;  /* Multi line mode. Default is single line. */
static int history_max_len = LINENOISE_DEFAULT_HISTORY_MAX_LEN;
static int history_len = 0;
static char **history = NULL;

/* The linenoiseState structure represents the state during line editing.
 * We pass this state to functions implementing specific editing
 * functionalities. */
struct linenoiseState {
	char buf[LINENOISE_MAX_LINE];  /* Edited line buffer. */
	size_t buflen;	  /* Edited line buffer size. */
	char prompt[LINENOISE_MAX_PROMPT]; /* Prompt to display. */
	size_t plen;		/* Prompt length. */
	size_t pos;		 /* Current cursor position. */
	size_t oldpos;	  /* Previous refresh cursor position. */
	size_t len;		 /* Current edited line length. */
	size_t cols;		/* Number of columns in terminal. */
	size_t maxrows;	 /* Maximum num of rows used so far (multiline mode) */
	int history_index;  /* The history index we are currently editing. */
};

enum KEY_ACTION{
	KEY_NULL = 0,		/* NULL */
	CTRL_A = 1,		 /* Ctrl+a */
	CTRL_B = 2,		 /* Ctrl-b */
	CTRL_C = 3,		 /* Ctrl-c */
	CTRL_D = 4,		 /* Ctrl-d */
	CTRL_E = 5,		 /* Ctrl-e */
	CTRL_F = 6,		 /* Ctrl-f */
	CTRL_H = 8,		 /* Ctrl-h */
	TAB = 9,			/* Tab */
	CTRL_K = 11,		/* Ctrl+k */
	CTRL_L = 12,		/* Ctrl+l */
	ENTER = 10,		 /* Enter */
	CTRL_N = 14,		/* Ctrl-n */
	CTRL_P = 16,		/* Ctrl-p */
	CTRL_T = 20,		/* Ctrl-t */
	CTRL_U = 21,		/* Ctrl+u */
	CTRL_W = 23,		/* Ctrl+w */
	ESC = 27,		   /* Escape */
	BACKSPACE =  127	/* Backspace */
};

static int linenoiseHistoryAdd(const char *line);
static void refreshLine(struct linenoiseState *l);

/* Debugging macro. */
#if 0
FILE *lndebug_fp = NULL;
#define lndebug(...) \
	do { \
		if (lndebug_fp == NULL) { \
			lndebug_fp = fopen("/tmp/lndebug.txt","a"); \
			fprintf(lndebug_fp, \
			"[%d %d %d] p: %d, rows: %d, rpos: %d, max: %d, oldmax: %d\n", \
			(int)l->len,(int)l->pos,(int)l->oldpos,plen,rows,rpos, \
			(int)l->maxrows,old_rows); \
		} \
		fprintf(lndebug_fp, ", " __VA_ARGS__); \
		fflush(lndebug_fp); \
	} while (0)
#else
#define lndebug(fmt, ...)
#endif

/* ======================= Low level terminal handling ====================== */

/* Set if to use or not the multi line mode. */
static void linenoiseSetMultiLine(int ml) {
	mlmode = ml;
}


/* Use the ESC [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the
 * cursor. */
static int getCursorPosition() {
	char buf[32];
	int cols, rows;
	unsigned int i = 0;

	/* Report cursor location */
	fprintf(stdout, "\x1b[6n");

	/* Read the response: ESC [ rows ; cols R */
	while (i < sizeof(buf)-1) {
		if (fread(buf+i, 1, 1, stdin) != 1) break;
		if (buf[i] == 'R') break;
		i++;
	}
	buf[i] = '\0';
	/* Parse it. */
	if (buf[0] != ESC || buf[1] != '[') return -1;
	if (sscanf(buf+2,"%d;%d",&rows,&cols) != 2) return -1;
	return cols;
}

/* Try to get the number of columns in the current terminal, or assume 80
 * if it fails. */
static int getColumns() {
	int start, cols;

	/* Get the initial position so we can restore it later. */
	start = getCursorPosition();
	if (start == -1) goto failed;

	/* Go to right margin and get position. */
	if (fwrite("\x1b[999C", 1, 6, stdout) != 6) goto failed;
	cols = getCursorPosition();
	if (cols == -1) goto failed;

	/* Restore position. */
	if (cols > start) {
		char seq[32];
		snprintf(seq,32,"\x1b[%dD",cols-start);
		if (fwrite(seq, 1, strlen(seq), stdout) == -1) {
			/* Can't recover... */
		}
	}
	return cols;

failed:
	return 80;
}

/* Clear the screen. Used to handle ctrl+l */
static void linenoiseClearScreen(void) {
	fprintf(stdout,"\x1b[H\x1b[2J");
}

/* =========================== Line editing ================================= */

/* We define a very simple "append buffer" structure, that is an heap
 * allocated string where we can append to. This is useful in order to
 * write all the escape sequences in a buffer and flush them to the standard
 * output in a single call, to avoid flickering effects. */
struct abuf {
	char *b;
	int len;
};

static void abInit(struct abuf *ab) {
	ab->b = NULL;
	ab->len = 0;
}

static void abAppend(struct abuf *ab, const char *s, int len) {
	char *new = realloc(ab->b,ab->len+len);

	if (new == NULL) return;
	memcpy(new+ab->len,s,len);
	ab->b = new;
	ab->len += len;
}

static void abFree(struct abuf *ab) {
	free(ab->b);
}

/* Single line low level line refresh.
 *
 * Rewrite the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal. */
static void refreshSingleLine(struct linenoiseState *l) {
	char seq[64];
	size_t plen = l->plen;
	char *buf = l->buf;
	size_t len = l->len;
	size_t pos = l->pos;
	struct abuf ab;

	while((plen+pos) >= l->cols) {
		buf++;
		len--;
		pos--;
	}
	while (plen+len > l->cols) {
		len--;
	}

	abInit(&ab);
	/* Cursor to left edge */
	snprintf(seq,64,"\r");
	abAppend(&ab,seq,strlen(seq));
	/* Write the prompt and the current buffer content */
	abAppend(&ab,l->prompt,strlen(l->prompt));
	abAppend(&ab,buf,len);
	/* Erase to right */
	snprintf(seq,64,"\x1b[0K");
	abAppend(&ab,seq,strlen(seq));
	/* Move cursor to original position. */
	snprintf(seq,64,"\r\x1b[%dC", (int)(pos+plen));
	abAppend(&ab,seq,strlen(seq));
	if (fwrite(ab.b, ab.len, 1, stdout) == -1) {} /* Can't recover from write error. */
	abFree(&ab);
}

/* Multi line low level line refresh.
 *
 * Rewrite the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal. */
static void refreshMultiLine(struct linenoiseState *l) {
	char seq[64];
	int plen = l->plen;
	int rows = (plen+l->len+l->cols-1)/l->cols; /* rows used by current buf. */
	int rpos = (plen+l->oldpos+l->cols)/l->cols; /* cursor relative row. */
	int rpos2; /* rpos after refresh. */
	int col; /* colum position, zero-based. */
	int old_rows = l->maxrows;
	int j;
	struct abuf ab;

	/* Update maxrows if needed. */
	if (rows > (int)l->maxrows) l->maxrows = rows;

	/* First step: clear all the lines used before. To do so start by
	 * going to the last row. */
	abInit(&ab);
	if (old_rows-rpos > 0) {
		lndebug("go down %d", old_rows-rpos);
		snprintf(seq,64,"\x1b[%dB", old_rows-rpos);
		abAppend(&ab,seq,strlen(seq));
	}

	/* Now for every row clear it, go up. */
	for (j = 0; j < old_rows-1; j++) {
		lndebug("clear+up");
		snprintf(seq,64,"\r\x1b[0K\x1b[1A");
		abAppend(&ab,seq,strlen(seq));
	}

	/* Clean the top line. */
	lndebug("clear");
	snprintf(seq,64,"\r\x1b[0K");
	abAppend(&ab,seq,strlen(seq));

	/* Write the prompt and the current buffer content */
	abAppend(&ab,l->prompt,strlen(l->prompt));
	abAppend(&ab,l->buf,l->len);

	/* If we are at the very end of the screen with our prompt, we need to
	 * emit a newline and move the prompt to the first column. */
	if (l->pos &&
		l->pos == l->len &&
		(l->pos+plen) % l->cols == 0)
	{
		lndebug("<newline>");
		abAppend(&ab,"\n",1);
		snprintf(seq,64,"\r");
		abAppend(&ab,seq,strlen(seq));
		rows++;
		if (rows > (int)l->maxrows) l->maxrows = rows;
	}

	/* Move cursor to right position. */
	rpos2 = (plen+l->pos+l->cols)/l->cols; /* current cursor relative row. */
	lndebug("rpos2 %d", rpos2);

	/* Go up till we reach the expected positon. */
	if (rows-rpos2 > 0) {
		lndebug("go-up %d", rows-rpos2);
		snprintf(seq,64,"\x1b[%dA", rows-rpos2);
		abAppend(&ab,seq,strlen(seq));
	}

	/* Set column. */
	col = (plen+(int)l->pos) % (int)l->cols;
	lndebug("set col %d", 1+col);
	if (col)
		snprintf(seq,64,"\r\x1b[%dC", col);
	else
		snprintf(seq,64,"\r");
	abAppend(&ab,seq,strlen(seq));

	lndebug("\n");
	l->oldpos = l->pos;

	if (fwrite(ab.b,ab.len,1,stdout) == -1) {} /* Can't recover from write error. */
	abFree(&ab);
}

/* Calls the two low level functions refreshSingleLine() or
 * refreshMultiLine() according to the selected mode. */
static void refreshLine(struct linenoiseState *l) {
	if (mlmode)
		refreshMultiLine(l);
	else
		refreshSingleLine(l);
}

/* Insert the character 'c' at cursor current position.
 *
 * On error writing to the terminal -1 is returned, otherwise 0. */
static int linenoiseEditInsert(struct linenoiseState *l, char c) {
	if (l->len < l->buflen) {
		if (l->len == l->pos) {
			l->buf[l->pos] = c;
			l->pos++;
			l->len++;
			l->buf[l->len] = '\0';
			if (!mlmode && l->plen+l->len < l->cols) {
				/* Avoid a full update of the line in the
				 * trivial case. */
				if (fwrite(&c,1,1,stdout) == -1) return -1;
			} else {
				refreshLine(l);
			}
		} else {
			memmove(l->buf+l->pos+1,l->buf+l->pos,l->len-l->pos);
			l->buf[l->pos] = c;
			l->len++;
			l->pos++;
			l->buf[l->len] = '\0';
			refreshLine(l);
		}
	}
	return 0;
}

/* Move cursor on the left. */
static void linenoiseEditMoveLeft(struct linenoiseState *l) {
	if (l->pos > 0) {
		l->pos--;
		refreshLine(l);
	}
}

/* Move cursor on the right. */
static void linenoiseEditMoveRight(struct linenoiseState *l) {
	if (l->pos != l->len) {
		l->pos++;
		refreshLine(l);
	}
}

/* Move cursor to the start of the line. */
static void linenoiseEditMoveHome(struct linenoiseState *l) {
	if (l->pos != 0) {
		l->pos = 0;
		refreshLine(l);
	}
}

/* Move cursor to the end of the line. */
static void linenoiseEditMoveEnd(struct linenoiseState *l) {
	if (l->pos != l->len) {
		l->pos = l->len;
		refreshLine(l);
	}
}

/* Substitute the currently edited line with the next or previous history
 * entry as specified by 'dir'. */
#define LINENOISE_HISTORY_NEXT 0
#define LINENOISE_HISTORY_PREV 1
static void linenoiseEditHistoryNext(struct linenoiseState *l, int dir) {
	if (history_len > 1) {
		/* Update the current history entry before to
		 * overwrite it with the next one. */
		free(history[history_len - 1 - l->history_index]);
		history[history_len - 1 - l->history_index] = strdup(l->buf);
		/* Show the new entry */
		l->history_index += (dir == LINENOISE_HISTORY_PREV) ? 1 : -1;
		if (l->history_index < 0) {
			l->history_index = 0;
			return;
		} else if (l->history_index >= history_len) {
			l->history_index = history_len-1;
			return;
		}
		strncpy(l->buf,history[history_len - 1 - l->history_index],l->buflen);
		l->buf[l->buflen-1] = '\0';
		l->len = l->pos = strlen(l->buf);
		refreshLine(l);
	}
}

/* Delete the character at the right of the cursor without altering the cursor
 * position. Basically this is what happens with the "Delete" keyboard key. */
static void linenoiseEditDelete(struct linenoiseState *l) {
	if (l->len > 0 && l->pos < l->len) {
		memmove(l->buf+l->pos,l->buf+l->pos+1,l->len-l->pos-1);
		l->len--;
		l->buf[l->len] = '\0';
		refreshLine(l);
	}
}

/* Backspace implementation. */
static void linenoiseEditBackspace(struct linenoiseState *l) {
	if (l->pos > 0 && l->len > 0) {
		memmove(l->buf+l->pos-1,l->buf+l->pos,l->len-l->pos);
		l->pos--;
		l->len--;
		l->buf[l->len] = '\0';
		refreshLine(l);
	}
}

/* Delete the previosu word, maintaining the cursor at the start of the
 * current word. */
static void linenoiseEditDeletePrevWord(struct linenoiseState *l) {
	size_t old_pos = l->pos;
	size_t diff;

	while (l->pos > 0 && l->buf[l->pos-1] == ' ')
		l->pos--;
	while (l->pos > 0 && l->buf[l->pos-1] != ' ')
		l->pos--;
	diff = old_pos - l->pos;
	memmove(l->buf+l->pos,l->buf+old_pos,l->len-old_pos+1);
	l->len -= diff;
	refreshLine(l);
}


/* This function is the core of the line editing capability of linenoise.
 * It expects 'fd' to be already in "raw mode" so that every key pressed
 * will be returned ASAP to read().
 *
 * The resulting string is put into 'buf' when the user type enter, or
 * when ctrl+d is typed.
 *
 * The function returns the length of the current buffer. */
#define NOLINE -8881
static int linenoiseEdit(struct linenoiseState *l)
{
	while(1) {
		char c;
		int nread;
		char seq[3];

		nread = fread(&c, 1, 1, stdin);
		if (nread <= 0) return NOLINE;

		switch(c) {
		case ENTER:	/* enter */
			history_len--;
			free(history[history_len]);
			if (mlmode) linenoiseEditMoveEnd(l);
			return (int)l->len;
		case CTRL_C:	 /* ctrl-c */
			errno = EAGAIN;
			return -1;
		case BACKSPACE:   /* backspace */
		case 8:	 /* ctrl-h */
			linenoiseEditBackspace(l);
			break;
		case CTRL_D:	 /* ctrl-d, remove char at right of cursor, or if the
							line is empty, act as end-of-file. */
			if (l->len > 0) {
				linenoiseEditDelete(l);
			} else {
				history_len--;
				free(history[history_len]);
				return -1;
			}
			break;
		case CTRL_T:	/* ctrl-t, swaps current character with previous. */
			if (l->pos > 0 && l->pos < l->len) {
				int aux = l->buf[l->pos-1];
				l->buf[l->pos-1] = l->buf[l->pos];
				l->buf[l->pos] = aux;
				if (l->pos != l->len-1) l->pos++;
				refreshLine(l);
			}
			break;
		case CTRL_B:	 /* ctrl-b */
			linenoiseEditMoveLeft(l);
			break;
		case CTRL_F:	 /* ctrl-f */
			linenoiseEditMoveRight(l);
			break;
		case CTRL_P:	/* ctrl-p */
			linenoiseEditHistoryNext(l, LINENOISE_HISTORY_PREV);
			break;
		case CTRL_N:	/* ctrl-n */
			linenoiseEditHistoryNext(l, LINENOISE_HISTORY_NEXT);
			break;
		case ESC:	/* escape sequence */
			/* Read the next two bytes representing the escape sequence. */
			if (fread(seq, 1, 2, stdin) < 2) break;

			/* ESC [ sequences. */
			if (seq[0] == '[') {
				if (seq[1] >= '0' && seq[1] <= '9') {
					/* Extended escape, read additional byte. */
					if (fread(seq+2, 1, 1, stdin) == -1) break;
					if (seq[2] == '~') {
						switch(seq[1]) {
						case '3': /* Delete key. */
							linenoiseEditDelete(l);
							break;
						}
					}
				} else {
					switch(seq[1]) {
					case 'A': /* Up */
						linenoiseEditHistoryNext(l, LINENOISE_HISTORY_PREV);
						break;
					case 'B': /* Down */
						linenoiseEditHistoryNext(l, LINENOISE_HISTORY_NEXT);
						break;
					case 'C': /* Right */
						linenoiseEditMoveRight(l);
						break;
					case 'D': /* Left */
						linenoiseEditMoveLeft(l);
						break;
					case 'H': /* Home */
						linenoiseEditMoveHome(l);
						break;
					case 'F': /* End*/
						linenoiseEditMoveEnd(l);
						break;
					}
				}
			}

			/* ESC O sequences. */
			else if (seq[0] == 'O') {
				switch(seq[1]) {
				case 'H': /* Home */
					linenoiseEditMoveHome(l);
					break;
				case 'F': /* End*/
					linenoiseEditMoveEnd(l);
					break;
				}
			}
			break;
		default:
			if (linenoiseEditInsert(l,c)) return -1;
			break;
		case CTRL_U: /* Ctrl+u, delete the whole line. */
			l->buf[0] = '\0';
			l->pos = l->len = 0;
			refreshLine(l);
			break;
		case CTRL_K: /* Ctrl+k, delete from current to end of line. */
			l->buf[l->pos] = '\0';
			l->len = l->pos;
			refreshLine(l);
			break;
		case CTRL_A: /* Ctrl+a, go to the start of the line */
			linenoiseEditMoveHome(l);
			break;
		case CTRL_E: /* ctrl+e, go to the end of the line */
			linenoiseEditMoveEnd(l);
			break;
		case CTRL_L: /* ctrl+l, clear screen */
			linenoiseClearScreen();
			refreshLine(l);
			break;
		case CTRL_W: /* ctrl+w, delete previous word */
			linenoiseEditDeletePrevWord(l);
			break;
		}
	}
	return l->len;
}

static void sanitize(char* src) {
	char* dst = src;
	for (int c = *src; c != 0; src++, c = *src) {
		if (isprint(c)) {
			*dst = c;
			++dst;
		}
	}
	*dst = 0;
}

static void setup(struct linenoiseState *l) {
	/* Populate the linenoise state that we pass to functions implementing
	 * specific editing functionalities. */
	l->buflen = sizeof(l->buf);
	/* l->prompt setup elsewhere... */
	l->plen = strlen(l->prompt);
	l->oldpos = l->pos = 0;
	l->len = 0;
	l->maxrows = 0;
	l->history_index = 0;

	/* Buffer starts empty. */
	l->buf[0] = '\0';
	l->buflen--; /* Make sure there is always space for the nulterm */

	/* The latest history entry is always our current buffer, that
	 * initially is just an empty string. */
	linenoiseHistoryAdd("");

	int flags = fcntl(STDIN_FILENO, F_GETFL);
	flags &= ~O_NONBLOCK;
	int res = fcntl(STDIN_FILENO, F_SETFL, flags);res=res;
	l->cols = getColumns();

	int pos1 = getCursorPosition();
	if (fwrite(l->prompt,l->plen,1,stdout) == -1) {
		return;
	}
	int pos2 = getCursorPosition();
	if (pos1 >= 0 && pos2 >= 0) {
		l->plen = pos2 - pos1;
	}
	flags |= O_NONBLOCK;
	res = fcntl(STDIN_FILENO, F_SETFL, flags);res=res;
}


/* ================================ History ================================= */

//static void linenoiseHistoryFree() {
//	if (history) {
//		for (int j = 0; j < history_len; j++) {
//			free(history[j]);
//		}
//		free(history);
//	}
//	history = NULL;
//}

/* This is the API call to add a new entry in the linenoise history.
 * It uses a fixed array of char pointers that are shifted (memmoved)
 * when the history max length is reached in order to remove the older
 * entry and make room for the new one, so it is not exactly suitable for huge
 * histories, but will work well for a few hundred of entries.
 *
 * Using a circular buffer is smarter, but a bit more complex to handle. */
static int linenoiseHistoryAdd(const char *line) {
	char *linecopy;

	if (history_max_len == 0) return 0;

	/* Initialization on first call. */
	if (history == NULL) {
		history = malloc(sizeof(char*)*history_max_len);
		if (history == NULL) return 0;
		memset(history,0,(sizeof(char*)*history_max_len));
	}

	/* Don't add duplicated lines. */
	if (history_len && !strcmp(history[history_len-1], line)) return 0;

	/* Add an heap allocated copy of the line in the history.
	 * If we reached the max length, remove the older line. */
	linecopy = strdup(line);
	if (!linecopy) return 0;
	if (history_len == history_max_len) {
		free(history[0]);
		memmove(history,history+1,sizeof(char*)*(history_max_len-1));
		history_len--;
	}
	history[history_len] = linecopy;
	history_len++;
	return 1;
}

/* Set the maximum length for the history. This function can be called even
 * if there is already some history, the function will make sure to retain
 * just the latest 'len' elements if the new history length value is smaller
 * than the amount of items already inside the history. */
static int linenoiseHistorySetMaxLen(int len) {
	char **new;

	if (len < 1) return 0;
	if (history) {
		int tocopy = history_len;

		new = malloc(sizeof(char*)*len);
		if (new == NULL) return 0;

		/* If we can't copy everything, free the elements we'll not use. */
		if (len < tocopy) {
			int j;

			for (j = 0; j < tocopy-len; j++) free(history[j]);
			tocopy = len;
		}
		memset(new,0,sizeof(char*)*len);
		memcpy(new,history+(history_len-tocopy), sizeof(char*)*tocopy);
		free(history);
		history = new;
	}
	history_max_len = len;
	if (history_len > history_max_len)
		history_len = history_max_len;
	return 1;
}

static struct linenoiseState *ourstate=0;

static void setPrompt(const char *prompt) {
	struct linenoiseState *l = ourstate;
	snprintf(l->prompt, sizeof(l->prompt), "%s", prompt);
}

static duk_ret_t js_linenoise_init(duk_context *ctx) {
	LOGD(">> init_linenoise");
	ourstate = calloc(1, sizeof(*ourstate));
	if(!ourstate) {
		LOGE("No memory for ourstate");
		return 0;
	}

	/* Disable buffering on stdin and stdout */
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);

	/* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
	esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
	/* Move the caret to the beginning of the next line on '\n' */
	esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

	/* Install UART driver for interrupt-driven reads and writes */
	ESP_ERROR_CHECK( uart_driver_install(CONFIG_CONSOLE_UART_NUM,
			256, 0, 0, NULL, 0) );

	/* Tell VFS to use UART driver */
	esp_vfs_dev_uart_use_driver(CONFIG_CONSOLE_UART_NUM);

	/* Initialize the console */
	esp_console_config_t console_config = {
			.max_cmdline_args = 8,
			.max_cmdline_length = 256,
	};
	ESP_ERROR_CHECK( esp_console_init(&console_config) );

	/* Enable multiline editing. If not set, long commands will scroll within
	 * single line.
	 */
	linenoiseSetMultiLine(1);

	/* Set command history size */
	linenoiseHistorySetMaxLen(100);
	setPrompt("esp32> ");

	LOGD("<< init_linenoise");
	return 0;
} // init_linenoise

/**
 * Set prompt
 * [0] - prompt <String>
 */
static duk_ret_t js_linenoise_setprompt(duk_context *ctx) {
	if(!ourstate) return 0;
	struct linenoiseState *l = ourstate;
	snprintf(l->prompt, sizeof(l->prompt), "%s", duk_get_string(ctx, 0));
	return 0;
}

/**
 * Poll function, call periodically to process typed keys
 * Returns <String> if we got a line
 */
static duk_ret_t js_linenoise_poll(duk_context *ctx) {
	static int ready = 0;
	if(!ourstate) return 0;
	void prepare(void) {
		setup(ourstate);
	}
	if(!ready) {
		prepare();
		ready = 1;
	}
	int count = linenoiseEdit(ourstate);
	if (count >= 0) {
	if(count>=0)
		fputc('\n', stdout);
		char *line = ourstate->buf;
		sanitize(line);
//		LOGI("Line %s\n", line);
		linenoiseHistoryAdd(line);
		duk_push_string(ctx, line);
		ready = 0;
		return 1;
	}
	return 0;
}

/**
 * Print a line
 * [0] - line <String>
 */
static duk_ret_t js_linenoise_print(duk_context *ctx) {
	printf("%s\n", duk_get_string(ctx, 0));
	return 0;
}

/**
 * Add native methods to the LINENOISE object.
 * [0] - LINENOISE Object
 */
duk_ret_t ModuleLinenoise(duk_context *ctx) {
	ADD_FUNCTION("init", js_linenoise_init, 0);
	ADD_FUNCTION("setPrompt", js_linenoise_setprompt, 1);
	ADD_FUNCTION("poll", js_linenoise_poll, 0);
	ADD_FUNCTION("print", js_linenoise_print, 1);
	return 0;
} // ModuleLinenoise
//#endif

