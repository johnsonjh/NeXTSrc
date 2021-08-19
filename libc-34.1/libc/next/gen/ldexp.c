double ldexp(v, e)
	double v;
	int e;
{
	double	exp;
	int	neg = 0;

	exp = 1.0;
	if (e < 0) {
		neg = 1;
		e = -e;
	}

	while (e) {
		exp = exp * 2.0;
		e--;
	}

	if (neg)
		return(v / (double)exp);
	else
		return(v * (double)exp);
}
