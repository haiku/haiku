typedef union {
	char	 *s_value;
	char	  c_value;
	int	  i_value;
	arg_list *a_value;
       } YYSTYPE;
#define	ENDOFLINE	257
#define	AND	258
#define	OR	259
#define	NOT	260
#define	STRING	261
#define	NAME	262
#define	NUMBER	263
#define	ASSIGN_OP	264
#define	REL_OP	265
#define	INCR_DECR	266
#define	Define	267
#define	Break	268
#define	Quit	269
#define	Length	270
#define	Return	271
#define	For	272
#define	If	273
#define	While	274
#define	Sqrt	275
#define	Else	276
#define	Scale	277
#define	Ibase	278
#define	Obase	279
#define	Auto	280
#define	Read	281
#define	Warranty	282
#define	Halt	283
#define	Last	284
#define	Continue	285
#define	Print	286
#define	Limits	287
#define	UNARY_MINUS	288
#define	HistoryVar	289


extern YYSTYPE yylval;
