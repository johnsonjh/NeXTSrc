/*	@(#)func.c	1.2	*/
/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"

freefunc(n)
	struct namnod 	*n;
{
	freetree((struct trenod *)(n->namenv));
}


freetree(t)
	register struct trenod *t;
{
	if (t)
	{
		register int type;

		if (t->tretyp & CNTMSK)
		{
			t->tretyp--;
			return;
		}

		type = t->tretyp & COMMSK;

		switch (type)
		{
			case TFND:
				free(fndptr(t)->fndnam);
				freetree(fndptr(t)->fndval);
				break;

			case TCOM:
				freeio(comptr(t)->comio);
				free_arg(comptr(t)->comarg);
				free_arg(comptr(t)->comset);
				break;

			case TFORK:
				freeio(forkptr(t)->forkio);
				freetree(forkptr(t)->forktre);
				break;

			case TPAR:
				freetree(parptr(t)->partre);
				break;

			case TFIL:
			case TLST:
			case TAND:
			case TORF:
				freetree(lstptr(t)->lstlef);
				freetree(lstptr(t)->lstrit);
				break;

			case TFOR:
			{
				struct fornod *f = (struct fornod *)t;

				free(f->fornam);
				freetree(f->fortre);
				if (f->forlst)
				{
					freeio(f->forlst->comio);
					free_arg(f->forlst->comarg);
					free_arg(f->forlst->comset);
					free(f->forlst);
				}
			}
			break;

			case TWH:
			case TUN:
				freetree(whptr(t)->whtre);
				freetree(whptr(t)->dotre);
				break;

			case TIF:
				freetree(ifptr(t)->iftre);
				freetree(ifptr(t)->thtre);
				freetree(ifptr(t)->eltre);
				break;

			case TSW:
				free(swptr(t)->swarg);
				freereg(swptr(t)->swlst);
				break;
		}
		free(t);
	}
}

free_arg(argp)
	register struct argnod 	*argp;
{
	register struct argnod 	*sav;

	while (argp)
	{
		sav = argp->argnxt;
		free(argp);
		argp = sav;
	}
}


freeio(iop)
	register struct ionod *iop;
{
	register struct ionod *sav;

	while (iop)
	{
		if (iop->iofile & IODOC)
		{

#ifdef DEBUG
			prs("unlinking ");
			prs(iop->ioname);
			newline();
#endif

			unlink(iop->ioname);

			if (fiotemp == iop)
				fiotemp = iop->iolst;
			else
			{
				struct ionod *fiop = fiotemp;

				while (fiop->iolst != iop)
					fiop = fiop->iolst;
	
				fiop->iolst = iop->iolst;
			}
		}
		free(iop->ioname);
		free(iop->iolink);
		sav = iop->ionxt;
		free(iop);
		iop = sav;
	}
}


freereg(regp)
	register struct regnod 	*regp;
{
	register struct regnod 	*sav;

	while (regp)
	{
		free_arg(regp->regptr);
		freetree(regp->regcom);
		sav = regp->regnxt;
		free(regp);
		regp = sav;
	}
}


prf(t)
	register struct trenod	*t;
{
	sigchk();

	if (t)
	{
		register int	type;

		type = t->tretyp & COMMSK;

		switch(type)
		{
			case TFND:
			{
				register struct fndnod *f = (struct fndnod *)t;

				prs_buff(f->fndnam);
				prs_buff(sfuncstr);	/* DAG -- made strings sharable */
				prf(f->fndval);
				prs_buff(efuncstr);	/* DAG */
				break;
			}

			case TCOM:
			{
				prarg(comptr(t)->comset);
				prarg(comptr(t)->comarg);
				prio(comptr(t)->comio);
				break;
			}

			case TFORK:
				prf(forkptr(t)->forktre);
				prio(forkptr(t)->forkio);
				if (forkptr(t)->forktyp & FAMP)
					prs_buff(amperstr);	/* DAG */
				break;

			case TPAR:
				prs_buff(lparstr);	/* DAG */
				prf(parptr(t)->partre);
				prs_buff(rparstr);	/* DAG */
				break;

			case TFIL:
				prf(lstptr(t)->lstlef);
				prs_buff(pipestr);	/* DAG */
				prf(lstptr(t)->lstrit);
				break;

			case TLST:
				prf(lstptr(t)->lstlef);
				prc_buff(NL);
				prf(lstptr(t)->lstrit);
				break;

			case TAND:
				prf(lstptr(t)->lstlef);
				prs_buff(andstr);	/* DAG */
				prf(lstptr(t)->lstrit);
				break;

			case TORF:
				prf(lstptr(t)->lstlef);
				prs_buff(orstr);	/* DAG */
				prf(lstptr(t)->lstrit);
				break;

			case TFOR:
				{
					register struct argnod	*arg;
					register struct fornod 	*f = (struct fornod *)t;

					prs_buff(forstr);	/* DAG */
					prs_buff(f->fornam);

					if (f->forlst)
					{
						arg = f->forlst->comarg;
						prs_buff(instr);	/* DAG */

						while(arg != ENDARGS)
						{
							prc_buff(SP);
							prs_buff(arg->argval);
							arg = arg->argnxt;
						}
					}

					prs_buff(dostr);	/* DAG */
					prf(f->fortre);
					prs_buff(donestr);	/* DAG */
				}
				break;

			case TWH:
			case TUN:
				if (type == TWH)
					prs_buff(whilestr);	/* DAG */
				else
					prs_buff(untilstr);	/* DAG */
				prf(whptr(t)->whtre);
				prs_buff(dostr);	/* DAG */
				prf(whptr(t)->dotre);
				prs_buff(donestr);	/* DAG */
				break;

			case TIF:
			{
				struct ifnod *f = (struct ifnod *)t;

				prs_buff(ifstr);	/* DAG */
				prf(f->iftre);
				prs_buff(thenstr);	/* DAG */
				prf(f->thtre);

				if (f->eltre)
				{
					prs_buff(elsestr);	/* DAG */
					prf(f->eltre);
				}

				prs_buff(fistr);	/* DAG */
				break;
			}

			case TSW:
				{
					register struct regnod 	*swl;

					prs_buff(casestr);	/* DAG */
					prs_buff(swptr(t)->swarg);
					prs_buff(instr);	/* DAG -- bug fix (was missing) */

					swl = swptr(t)->swlst;
					while(swl)
					{
						struct argnod	*arg = swl->regptr;

						prc_buff(NL);	/* DAG -- bug fix (was missing) */
						if (arg)
						{
							prs_buff(arg->argval);
							arg = arg->argnxt;
						}

						while(arg)
						{
							prs_buff(pipestr);	/* DAG */
							prs_buff(arg->argval);
							arg = arg->argnxt;
						}

						prc_buff(')');	/* DAG */
						prf(swl->regcom);
						prs_buff(dsemistr);	/* DAG */
						swl = swl->regnxt;
					}
					prs_buff(esacstr);	/* DAG -- bug fix (was missing) */
				}
				break;
			} 
		} 

	sigchk();
}

prarg(argp)
	register struct argnod	*argp;
{
	while (argp)
	{
		prs_buff(argp->argval);
		prc_buff(SP);
		argp=argp->argnxt;
	}
}


prio(iop)
	register struct ionod	*iop;
{
	register int	iof;
	register char	*ion;

	while (iop)
	{
		iof = iop->iofile;
		ion = iop->ioname;

		if (ion && *ion)	/* DAG -- added safety check */
		{
			prn_buff(iof & IOUFD);

			if (iof & IODOC)
				prs_buff(fromstr);	/* DAG */
			else if (iof & IOMOV)
			{
				if (iof & IOPUT)
					prs_buff(toastr);	/* DAG */
				else
					prs_buff(fromastr);	/* DAG */

			}
			else if ((iof & IOPUT) == 0)
				prc_buff('<');
			else if (iof & IOAPP)
				prs_buff(ontostr);	/* DAG */
			else
				prc_buff('>');

			prs_buff(ion);
			prc_buff(SP);
		}
		iop = iop->ionxt;
	}
}
