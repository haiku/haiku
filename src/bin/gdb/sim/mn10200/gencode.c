#include "mn10200_sim.h"

static void write_header PARAMS ((void));
static void write_opcodes PARAMS ((void));
static void write_template PARAMS ((void));

int
main (argc, argv)
     int argc;
     char *argv[];
{
  if ((argc > 1) && (strcmp (argv[1], "-h") == 0))
    write_header();
  else if ((argc > 1) && (strcmp (argv[1], "-t") == 0))
    write_template ();
  else
    write_opcodes();
  return 0;
}


static void
write_header ()
{
  struct mn10200_opcode *opcode;

  for (opcode = (struct mn10200_opcode *)mn10200_opcodes; opcode->name; opcode++)
    printf("void OP_%X PARAMS ((unsigned long, unsigned long));\t\t/* %s */\n",
	   opcode->opcode, opcode->name);
}


/* write_template creates a file all required functions, ready */
/* to be filled out */

static void
write_template ()
{
  struct mn10200_opcode *opcode;
  int i,j;

  printf ("#include \"mn10200_sim.h\"\n");
  printf ("#include \"simops.h\"\n");

  for (opcode = (struct mn10200_opcode *)mn10200_opcodes; opcode->name; opcode++)
    {
      printf("/* %s */\nvoid\nOP_%X (insn, extension)\n     unsigned long insn, extension;\n{\n", opcode->name, opcode->opcode);
	  
      /* count operands */
      j = 0;
      for (i = 0; i < 6; i++)
	{
	  int flags = mn10200_operands[opcode->operands[i]].flags;

	  if (flags)
	    j++;
	}
      switch (j)
	{
	case 0:
	  printf ("printf(\"   %s\\n\");\n", opcode->name);
	  break;
	case 1:
	  printf ("printf(\"   %s\\t%%x\\n\", OP[0]);\n", opcode->name);
	  break;
	case 2:
	  printf ("printf(\"   %s\\t%%x,%%x\\n\",OP[0],OP[1]);\n",
		  opcode->name);
	  break;
	case 3:
	  printf ("printf(\"   %s\\t%%x,%%x,%%x\\n\",OP[0],OP[1],OP[2]);\n",
		  opcode->name);
	  break;
	default:
	  fprintf (stderr,"Too many operands: %d\n", j);
	}
      printf ("}\n\n");
    }
}

static void
write_opcodes ()
{
  struct mn10200_opcode *opcode;
  int i, j;
  int numops;
  
  /* write out opcode table */
  printf ("#include \"mn10200_sim.h\"\n");
  printf ("#include \"simops.h\"\n\n");
  printf ("struct simops Simops[] = {\n");
  
  for (opcode = (struct mn10200_opcode *)mn10200_opcodes; opcode->name; opcode++)
    {
      int size;

      if (opcode->format == FMT_1)
	size = 1;
      else if (opcode->format == FMT_2 || opcode->format == FMT_4)
	size = 2;
      else if (opcode->format == FMT_3 || opcode->format == FMT_5)
	size = 3;
      else if (opcode->format == FMT_6)
	size = 4;
      else if (opcode->format == FMT_7)
	size = 5;
      else
	abort ();

      printf ("  { 0x%x,0x%x,OP_%X,%d,%d,",
	      opcode->opcode, opcode->mask, opcode->opcode,
	      size, opcode->format);
      
      /* count operands */
      j = 0;
      for (i = 0; i < 6; i++)
	{
	  int flags = mn10200_operands[opcode->operands[i]].flags;

	  if (flags)
	    j++;
	}
      printf ("%d,{",j);
	  
      j = 0;
      numops = 0;
      for (i = 0; i < 6; i++)
	{
	  int flags = mn10200_operands[opcode->operands[i]].flags;
	  int shift = mn10200_operands[opcode->operands[i]].shift;

	  if (flags)
	    {
	      if (j)
		printf (", ");
	      printf ("%d,%d,%d", shift,
		      mn10200_operands[opcode->operands[i]].bits,flags);
	      j = 1;
	      numops++;
	    }
	}

      switch (numops)
	{
	case 0:
	  printf ("0,0,0");
	case 1:
	  printf (",0,0,0");
	}

      printf ("}},\n");
    }
  printf ("{ 0,0,NULL,0,0,0,{0,0,0,0,0,0}},\n};\n");
}
