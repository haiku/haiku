#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

#ifndef YYSTYPE
typedef union {
  WORD_DESC *word;		/* the word that we read. */
  int number;			/* the number that we read. */
  WORD_LIST *word_list;
  COMMAND *command;
  REDIRECT *redirect;
  ELEMENT element;
  PATTERN_LIST *pattern;
} yystype;
# define YYSTYPE yystype
#endif
# define	IF	257
# define	THEN	258
# define	ELSE	259
# define	ELIF	260
# define	FI	261
# define	CASE	262
# define	ESAC	263
# define	FOR	264
# define	SELECT	265
# define	WHILE	266
# define	UNTIL	267
# define	DO	268
# define	DONE	269
# define	FUNCTION	270
# define	COND_START	271
# define	COND_END	272
# define	COND_ERROR	273
# define	IN	274
# define	BANG	275
# define	TIME	276
# define	TIMEOPT	277
# define	WORD	278
# define	ASSIGNMENT_WORD	279
# define	NUMBER	280
# define	ARITH_CMD	281
# define	ARITH_FOR_EXPRS	282
# define	COND_CMD	283
# define	AND_AND	284
# define	OR_OR	285
# define	GREATER_GREATER	286
# define	LESS_LESS	287
# define	LESS_AND	288
# define	LESS_LESS_LESS	289
# define	GREATER_AND	290
# define	SEMI_SEMI	291
# define	LESS_LESS_MINUS	292
# define	AND_GREATER	293
# define	LESS_GREATER	294
# define	GREATER_BAR	295
# define	yacc_EOF	296


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
