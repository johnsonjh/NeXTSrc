/* Print information on generated parser, for bison,
   Copyright (C) 1984, 1986, 1989 Free Software Foundation, Inc.

This file is part of Bison, the GNU Compiler Compiler.

Bison is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

Bison is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Bison; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */


#include <stdio.h>
#include "machine.h"
#include "new.h"
#include "files.h"
#include "gram.h"
#include "state.h"


extern char **tags;
extern int nstates;
extern short *accessing_symbol;
extern core **state_table;
extern shifts **shift_table;
extern errs **err_table;
extern reductions **reduction_table;
extern char *consistent;
extern char any_conflicts;
extern char *conflicts;

extern void conflict_log();
extern void verbose_conflict_log();
extern void print_reductions();

void print_token();
void print_state();
void print_core();
void print_actions();

void
terse()
{
  if (any_conflicts)
    {
      conflict_log();
    }
}


void
verbose()
{
  register int i;

  if (any_conflicts)
    verbose_conflict_log();

  fprintf(foutput, "\n\ntoken types:\n");
  print_token (-1, 0);
  if (translations)
    {
      for (i = 0; i <= max_user_token_number; i++)
	/* Don't mention all the meaningless ones.  */
	if (token_translations[i] != 2)
	  print_token (i, token_translations[i]);
    }
  else
    for (i = 1; i < ntokens; i++)
      print_token (i, i);

  for (i = 0; i < nstates; i++)
    {
      print_state(i);
    }
}


void
print_token(extnum, token)
int extnum, token;
{
  fprintf(foutput, " type %d is %s\n", extnum, tags[token]);
}


void
print_state(state)
int state;
{
  fprintf(foutput, "\n\nstate %d\n\n", state);
  print_core(state);
  print_actions(state);
}


void
print_core(state)
int state;
{
  register int i;
  register int k;
  register int rule;
  register core *statep;
  register short *sp;
  register short *sp1;

  statep = state_table[state];
  k = statep->nitems;

  if (k == 0) return;

  for (i = 0; i < k; i++)
    {
      sp1 = sp = ritem + statep->items[i];

      while (*sp > 0)
	sp++;

      rule = -(*sp);
      fprintf(foutput, "    %s  ->  ", tags[rlhs[rule]]);

      for (sp = ritem + rrhs[rule]; sp < sp1; sp++)
	{
	  fprintf(foutput, "%s ", tags[*sp]);
	}

      putc('.', foutput);

      while (*sp > 0)
	{
	  fprintf(foutput, " %s", tags[*sp]);
	  sp++;
	}

      fprintf (foutput, "   (%d)", rule);
      putc('\n', foutput);
    }

  putc('\n', foutput);
}


void
print_actions(state)
int state;
{
  register int i;
  register int k;
  register int state1;
  register int symbol;
  register shifts *shiftp;
  register errs *errp;
  register reductions *redp;
  register int rule;

  shiftp = shift_table[state];
  redp = reduction_table[state];
  errp = err_table[state];

  if (!shiftp && !redp)
    {
      fprintf(foutput, "    NO ACTIONS\n");
      return;
    }

  if (shiftp)
    {
      k = shiftp->nshifts;

      for (i = 0; i < k; i++)
	{
	  if (! shiftp->shifts[i]) continue;
	  state1 = shiftp->shifts[i];
	  symbol = accessing_symbol[state1];
/*	  if (ISVAR(symbol)) break;  */
	  fprintf(foutput, "    %-4s\tshift  %d\n", tags[symbol], state1);
	}

      if (i > 0)
	putc('\n', foutput);
    }
  else
    {
      i = 0;
      k = 0;
    }

  if (errp)
    {
      k = errp->nerrs;

      for (i = 0; i < k; i++)
	{
	  if (! errp->errs[i]) continue;
	  symbol = errp->errs[i];
	  fprintf(foutput, "    %-4s\terror (nonassociative)\n", tags[symbol]);
	}

      if (i > 0)
	putc('\n', foutput);
    }
  else
    {
      i = 0;
      k = 0;
    }

  if (consistent[state] && redp)
    {
      rule = redp->rules[0];
      symbol = rlhs[rule];
      fprintf(foutput, "    $default\treduce  %d  (%s)\n\n",
     	        rule, tags[symbol]);
    }
  else if (redp)
    {
      print_reductions(state);
    }

  if (i < k)
    {
      for (; i < k; i++)
	{
	  if (! shiftp->shifts[i]) continue;
	  state1 = shiftp->shifts[i];
	  symbol = accessing_symbol[state1];
	  fprintf(foutput, "    %-4s\tgoto  %d\n", tags[symbol], state1);
	}

      putc('\n', foutput);
    }
}
