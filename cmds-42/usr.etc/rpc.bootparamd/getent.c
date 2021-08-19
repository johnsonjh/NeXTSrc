useni()
{
	static int useit = -1;

	if (useit == -1){
		useit = _lu_running();
	}
	return (useit);
}

int
ni_bplookup(
	    char *client,
	    char ***values
	    )
{
	return (bootparams_getbyname(client, values));
}

