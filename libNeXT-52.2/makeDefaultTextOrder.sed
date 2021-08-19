#
# sed file for parsing kit file names out of spec file
# (all except for files with global data, which are handled especially
# carefully elsewhere)
#

/libappkit\/shlib_obj\/.*globals/d
/libappkit\/shlib_obj/s/.*libappkit/libappkit/
/libappkit\/shlib_obj/s/\.o.*/.o/
/libappkit\/shlib_obj/p

/libsoundkit\/shlib_obj\/.*globals/d
/libsoundkit\/shlib_obj/s/.*libsoundkit/libsoundkit/
/libsoundkit\/shlib_obj/s/\.o.*/.o/
/libsoundkit\/shlib_obj/p

/libdpsclient\/shlib_obj\/.*globals/d
/libdpsclient\/shlib_obj/s/.*libdpsclient/libdpsclient/
/libdpsclient\/shlib_obj/s/\.o.*/.o/
/libdpsclient\/shlib_obj/p

d
