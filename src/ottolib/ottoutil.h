#ifndef _OTTOUTIL_H_
#define _OTTOUTIL_H_

// stdin read buffer
#pragma pack(1)
typedef struct _dynbuf
{
	char    *buffer;
	char    *s;
	int     bufferlen;
	int     eob;
	int     line;
} DYNBUF;
#pragma pack()

void  output_xml(char *s);
char *copy_word(char *t, char *s);
void  copy_eol(char *t, char *s);
char *format_timestamp(time_t t);
char *format_duration(time_t t);
void  mkcode(char *t, char *s);
void  mkampcode(char *t, char *s);
int   xmltoi(char *source);
void  xmlcpy(char *t, char *s);
int   xmlchr(char *s, int c);
int   xmlcmp(char *s1, char *s2);
int   read_stdin(DYNBUF *b, char *prompt);
void  remove_jil_comments(DYNBUF *b);
void  advance_word(DYNBUF *b);
void  advance_jilword(DYNBUF *b);
void  regress_word(DYNBUF *b);

int   strwcmp(char *pTameText, char *pWildText);

#endif
//
// vim: ts=3 sw=3 ai
//
