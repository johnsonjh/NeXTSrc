/*
 * Serialization definitions
 * Copyright (C) 1989 by NeXT, Inc.
 */
ni_status ser_decode(FILE *, long, ni_object **);
ni_status ser_encode(ni_object *, char *, long *, FILE **);
ni_status ser_free(ni_object *);
ni_status ser_size(ni_object *, long *);
ni_status ser_fastencode(FILE *, ni_object *);
