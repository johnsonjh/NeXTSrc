#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>

useni()
{
	static int useit = -1;

	if (useit == -1){
		useit = _lu_running();
	}
	return (useit);
}

int
ni_getbyether(
	      struct ether_addr *en,
	      char **name,
	      struct in_addr *ip,
	      char **bootfile
	      )
{
	return (bootp_getbyether(en, name, ip, bootfile));
}

int
ni_getbyip(
	   struct in_addr *ip,
	   char **name,
	   struct ether_addr *en,
	   char **bootfile
	   )
{
	return (bootp_getbyip(en, name, ip, bootfile));
}

