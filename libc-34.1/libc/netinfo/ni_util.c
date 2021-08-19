/*
 * Utility routines for netinfo data structures
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <netinfo/ni.h>
#include <stdio.h>
#include "clib.h"
#include "mm.h"

extern char *strcpy(char *, const char *);


void
ni_idlist_insert(
	      ni_idlist *cl,
	      ni_index id,
	      ni_index where
	      )
{
	ni_index i;

	MM_GROW_ARRAY(cl->niil_val, cl->niil_len);
	for (i = cl->niil_len; i > where; i--) {
		cl->niil_val[i] = cl->niil_val[i - 1];
	}
	cl->niil_val[i] = id;
	cl->niil_len++;
}

int
ni_idlist_delete(
	      ni_idlist *idlist,
	      ni_index id
	      )
{
	ni_index j;
	ni_index i;

	for (i = 0; i < idlist->niil_len; i++) {
		if (idlist->niil_val[i] == id) {
			for (j = i + 1; j < idlist->niil_len; j++) {
				idlist->niil_val[j - 1] = idlist->niil_val[j];
			}
			MM_SHRINK_ARRAY(idlist->niil_val, idlist->niil_len--);
			return(1);
		}
	}
	return (0);
}


void
ni_idlist_free(
	       ni_idlist *idlist
	       )
{
	if (idlist->niil_val != NULL) {
		MM_FREE_ARRAY(idlist->niil_val, idlist->niil_len);
	}
	NI_INIT(idlist);
}

ni_idlist
ni_idlist_dup(
	   const ni_idlist idlist
	   )
{
	ni_idlist newlist;
	ni_index i;

	newlist.niil_len = idlist.niil_len;
	MM_ALLOC_ARRAY(newlist.niil_val, idlist.niil_len);
	for (i = 0; i < idlist.niil_len; i++) {
		newlist.niil_val[i] = idlist.niil_val[i];
	}
	return (newlist);
}

void
ni_proplist_insert(
		ni_proplist *pl,
		const ni_property prop,
		ni_index where
		)
{
	ni_index i;
	
	MM_GROW_ARRAY(pl->nipl_val, pl->nipl_len);
	for (i = pl->nipl_len; i > where; i--) {
		pl->nipl_val[i] = pl->nipl_val[i - 1];
	}
	pl->nipl_val[i] = ni_prop_dup(prop);
	pl->nipl_len++;
}

void
ni_proplist_delete(
		ni_proplist *pl,
		ni_index which
		)
{
	int i;

	ni_prop_free(&pl->nipl_val[which]);
	for (i = which + 1; i < pl->nipl_len; i++) {
		pl->nipl_val[i - 1] = pl->nipl_val[i];
	}
	MM_SHRINK_ARRAY(pl->nipl_val, pl->nipl_len--);
}

void
ni_proplist_free(
		 ni_proplist *pl
		 )
{
	ni_index i;

	if (pl->nipl_val == NULL) {
		return;
	}
	for (i = 0; i < pl->nipl_len; i++) {
		ni_prop_free(&pl->nipl_val[i]);
	}
	MM_FREE_ARRAY(pl->nipl_val, pl->nipl_len);
	NI_INIT(pl);
}

void
ni_proplist_list_free(
		 ni_proplist_list *pll
		 )
{
	ni_index i;

	if (pll->nipll_val == NULL) {
		return;
	}
	for (i = 0; i < pll->nipll_len; i++) {
		ni_proplist_free(&pll->nipll_val[i]);
	}
	MM_FREE_ARRAY(pll->nipll_val, pll->nipll_len);
	NI_INIT(pll);
}

ni_proplist
ni_proplist_dup(
	     const ni_proplist pl
	     )
{
	ni_proplist newlist;
	ni_index i;

	newlist.nipl_len = pl.nipl_len;
	MM_ALLOC_ARRAY(newlist.nipl_val, pl.nipl_len);
	for (i = 0; i < pl.nipl_len; i++) {
		newlist.nipl_val[i].nip_name = ni_name_dup(pl.nipl_val[i].nip_name);
		newlist.nipl_val[i].nip_val = ni_namelist_dup(pl.nipl_val[i].nip_val);
	}
	return (newlist);
}

ni_index
ni_proplist_match(
	       const ni_proplist pl,
	       ni_name_const pname,
	       ni_name_const pval
	   )
{
	ni_index i;
	ni_index j;
	ni_namelist nl;

	for (i = 0; i < pl.nipl_len; i++) {
		if (ni_name_match(pname, pl.nipl_val[i].nip_name)) {
			if (pval == NULL) {
				return (i);
			}
			nl = pl.nipl_val[i].nip_val;
			for (j = 0; j < nl.ninl_len; j++) {
				if (ni_name_match(pval, nl.ninl_val[j])) {
					return (i);
				}
			}
			break;
		}
	}
	return (NI_INDEX_NULL);
}


ni_property
ni_prop_dup(
	 const ni_property prop
	 )
{
	ni_property newprop;

	newprop.nip_name = ni_name_dup(prop.nip_name);
	newprop.nip_val = ni_namelist_dup(prop.nip_val);
	return (newprop);
}

void
ni_prop_free(
	     ni_property *prop
	     )
{
	ni_name_free(&prop->nip_name);
	ni_namelist_free(&prop->nip_val);
}

int
ni_name_match(
	   ni_name_const nm1,
	   ni_name_const nm2
	   )
{
	return (strcmp(nm1, nm2) == 0);
}

ni_name
ni_name_dup(
	 ni_name_const nm
	 )
{
	return (strcpy(malloc(strlen(nm) + 1), nm));
}


void
ni_name_free(
	     ni_name *nm
	     )
{
	if (*nm != NULL) {
		free(*nm);
		*nm = NULL;
	}
}

ni_namelist
ni_namelist_dup(
	     const ni_namelist nl
	     )
{
	ni_namelist newlist;
	ni_index i;

	newlist.ninl_len = nl.ninl_len;
	MM_ALLOC_ARRAY(newlist.ninl_val, newlist.ninl_len);
	for (i = 0; i < nl.ninl_len; i++) {
		newlist.ninl_val[i] = ni_name_dup(nl.ninl_val[i]);
	}
	return (newlist);
}

void
ni_namelist_free(
	      ni_namelist *nl
	      )
{
	ni_index i;

	if (nl->ninl_val == NULL) {
		return;
	}
	for (i = 0; i < nl->ninl_len; i++) {
		ni_name_free(&nl->ninl_val[i]);
	}
	MM_FREE_ARRAY(nl->ninl_val, nl->ninl_len);
	NI_INIT(nl);
}

void
ni_namelist_insert(
		ni_namelist *nl,
		ni_name_const nm,
		ni_index where
		)
{
	ni_index i;

	MM_GROW_ARRAY(nl->ninl_val, nl->ninl_len);
	for (i = nl->ninl_len; i > where; i--) {
		nl->ninl_val[i] = nl->ninl_val[i - 1];
	}
	nl->ninl_val[i] = ni_name_dup(nm);
	nl->ninl_len++;
}

void
ni_namelist_delete(
		ni_namelist *nl,
		ni_index which
		)
{
	int i;

	ni_name_free(&nl->ninl_val[which]);
	for (i = which + 1; i < nl-> ninl_len; i++) {
		nl->ninl_val[i - 1] = nl->ninl_val[i];
	}
	MM_SHRINK_ARRAY(nl->ninl_val, nl->ninl_len--);
}

ni_index
ni_namelist_match(
	       const ni_namelist nl,
	       ni_name_const nm
	       )
{
	ni_index i;
	
	for (i = 0; i < nl.ninl_len; i++) {
		if (ni_name_match(nl.ninl_val[i], nm)) {
			return (i);
		}
	}
	return (NI_INDEX_NULL);
}

void
ni_entrylist_insert(
		 ni_entrylist *el,
		 const ni_entry en
		 )
{
	ni_entry entry;

	MM_GROW_ARRAY(el->niel_val, el->niel_len);
	entry.id = en.id;
	if (en.names != NULL) {
		MM_ALLOC(entry.names);
		*entry.names = ni_namelist_dup(*en.names);
	} else {
		entry.names = NULL;
	}
	el->niel_val[el->niel_len++] = entry;
}

void
ni_entrylist_delete(
		 ni_entrylist *el,
		 ni_index which
		)
{
	int i;

	if (el->niel_val[which].names != NULL) {
		ni_namelist_free(el->niel_val[which].names);
	}
	for (i = which + 1; i < el-> niel_len; i++) {
		el->niel_val[i - 1] = el->niel_val[i];
	}
	MM_SHRINK_ARRAY(el->niel_val, el->niel_len--);
}

void
ni_entrylist_free(
		  ni_entrylist *el
		  )
{
	ni_index i;

	if (el->niel_val == NULL) {
		return;
	}
	for (i = 0; i < el->niel_len; i++) {
		if (el->niel_val[i].names != NULL) {
			ni_namelist_free(el->niel_val[i].names);
			MM_FREE(el->niel_val[i].names);
		}
	}
	MM_FREE_ARRAY(el->niel_val, el->niel_len);
	NI_INIT(el);
}



/*
 * We can do this without an addition to the protocol
 */
ni_status
ni_lookupprop(
	      void *ni,
	      ni_id *id,
	      ni_name_const pname,
	      ni_namelist *nl
	      )
{
	ni_status status;
	ni_namelist list;
	ni_index which;
	
	status = ni_listprops(ni, id, &list);
	if (status != NI_OK) {
		return (status);
	}
	which = ni_namelist_match(list, pname);
	ni_namelist_free(&list);
	if (which == NI_INDEX_NULL) {
		return (NI_NOPROP);
	}
	return (ni_readprop(ni, id, which, nl));
}

