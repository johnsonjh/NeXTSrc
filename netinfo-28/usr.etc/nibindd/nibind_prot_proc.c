/*
 * nibindd remote procedure implementation
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <sys/wait.h>
#include "clib.h"
#include "mm.h"
#include "system.h"

#define BIND_TIMEOUT 2
#define BIND_RETRIES 0

extern const char NETINFO_PROG[];
int waitreg;

/*
 * The list of registered servers
 */
struct {
	unsigned regs_len;
	nibind_registration *regs_val;
} regs;


typedef struct ni_serverinfo {
	int pid;
	ni_name name;
} ni_serverinfo;

ni_serverinfo *servers;
unsigned nservers;

void destroydir(ni_name);

void
storepid(
	 int pid,
	 ni_name name
	 )
{
	MM_GROW_ARRAY(servers, nservers);
	servers[nservers].pid = pid;
	servers[nservers].name = ni_name_dup(name);
	nservers++;
}

void
killchildren(void)
{
	int i;

	signal(SIGCHLD, SIG_IGN);
	for (i = 0; i < nservers; i++) {
		kill(servers[i].pid, SIGTERM);
	}
	exit(1);
}

ni_status
validate(
	 struct svc_req *req
	 )
{
	struct sockaddr_in *sin = svc_getcaller(req->rq_xprt);

	if (sys_ismyaddress(sin->sin_addr.s_addr) && 
	    sin->sin_port < IPPORT_RESERVED) {
		return (NI_OK);
	} 
	return (NI_PERM);
}

void
deletepid(
	  int pid
	  )
{
	int i;

	for (i = 0; i < nservers; i++) {
		if (servers[i].pid == pid) {
			(void)nibind_unregister_1(&servers[i].name, NULL);
			ni_name_free(&servers[i].name);
			for (i += 1; i < nservers; i++) {
				servers[i - 1] = servers[i];
				
			}
			MM_SHRINK_ARRAY(servers, nservers);
			nservers--;
			waitreg--;
			return;
		}
	}
}


void
register_done(
	      ni_name tag
	      )
{
	int i;

	for (i = 0; i < nservers; i++) {
		if (ni_name_match(servers[i].name, tag)) {
			waitreg--;
			return;
		}
	}
}

/*
 * Signal handler: must save and restore errno
 */
void
catchchild(void)
{
	int pid;
	union wait status;
	int save_errno;

	save_errno = errno;
	pid = wait3(&status, WNOHANG, NULL);
	if (pid > 0) {
		deletepid(pid);
	}
	errno = save_errno;
}

void *
nibind_ping_1(
	      void *arg,
	      struct svc_req *req
	      )
{
	return ((void *)~0);
}


ni_status *
nibind_register_1(
		  nibind_registration *arg,
		  struct svc_req *req
		  )
{
	static ni_status status;
	int i;

	status = validate(req);
	if (status != NI_OK) {
		return (&status);
	}
	for (i = 0; i < regs.regs_len; i++) {
		if (ni_name_match(arg->tag, regs.regs_val[i].tag)) {
			register_done(arg->tag);
			ni_name_free(&regs.regs_val[i].tag);
			regs.regs_val[i] = *arg;
			bzero(arg, sizeof(*arg));
			status = NI_OK;
			return (&status);
		}
	}
	register_done(arg->tag);
	MM_GROW_ARRAY(regs.regs_val, regs.regs_len);
	regs.regs_val[regs.regs_len++] = *arg;
	bzero(arg, sizeof(*arg));
	status = NI_OK;
	return (&status);
}


ni_status *
nibind_unregister_1(
		    ni_name *tag,
		    struct svc_req *req
		    )
{
	static ni_status status;
	ni_index i;

	if (req != NULL) {
		status = validate(req);
		if (status != NI_OK) {
			return (&status);
		}
	}
	for (i = 0; i < regs.regs_len; i++) {
		if (ni_name_match(*tag, regs.regs_val[i].tag)) {
			ni_name_free(&regs.regs_val[i].tag);
			regs.regs_val[i] = regs.regs_val[regs.regs_len - 1];
			MM_SHRINK_ARRAY(regs.regs_val, regs.regs_len);
			regs.regs_len--;
			break;
		}
	}
	status = NI_OK;
	return (&status);
}


nibind_getregister_res *
nibind_getregister_1(
		     ni_name *tag,
		     struct svc_req *req
		     )
{
	ni_index i;
	static nibind_getregister_res res;

	res.status = NI_NOTAG;
	for (i = 0; i < regs.regs_len; i++) {
		if (ni_name_match(*tag, regs.regs_val[i].tag)) {
			(res.nibind_getregister_res_u.addrs = 
			 regs.regs_val[i].addrs);
			res.status = NI_OK;
			break;
		}
	}
	return (&res);
}


nibind_listreg_res *
nibind_listreg_1(
	      void *arg,
	      struct svc_req *req
	      )
{
	static nibind_listreg_res res;

	res.status = NI_OK;
	res.nibind_listreg_res_u.regs.regs_len = regs.regs_len;
	res.nibind_listreg_res_u.regs.regs_val = regs.regs_val;
	return (&res);
}


ni_status *
nibind_createmaster_1(
		      ni_name *tag,
		      struct svc_req *req
		      )
{
	static ni_status status;
	ni_index i;
	int pid;

	status = validate(req);
	if (status != NI_OK) {
		return (&status);
	}
	for (i = 0; i < regs.regs_len; i++) {
		if (ni_name_match(*tag, regs.regs_val[i].tag)) {
			status = NI_DUPTAG;
			return (&status);
		}
	}
	pid = sys_spawn(NETINFO_PROG, "-m", *tag, 0);
	if (pid < 0) {
		status = NI_SYSTEMERR;
	} else {
		storepid(pid, *tag);
		status = NI_OK;
	}
	return (&status);
}

ni_status *
nibind_createclone_1(
		     nibind_clone_args *args,
		     struct svc_req *req
		     )
{
	struct in_addr addr;
	static ni_status status;
	ni_index i;
	int pid;


	status = validate(req);
	if (status != NI_OK) {
		return (&status);
	}
	addr.s_addr = args->master_addr;
	for (i = 0; i < regs.regs_len; i++) {
		if (ni_name_match(args->tag, regs.regs_val[i].tag)) {
			status = NI_DUPTAG;
			return (&status);
		}
	}
	pid = sys_spawn(NETINFO_PROG, "-c", args->master_name, inet_ntoa(addr),
		      args->master_tag, args->tag, 0);
	if (pid < 0) {
		status = NI_SYSTEMERR;
	} else {
		status = NI_OK;
		storepid(pid, args->tag);
	}
	return (&status);
}

ni_status *
nibind_destroydomain_1(
		     ni_name *tag,
		     struct svc_req *req
		     )
{
	static ni_status status;
	int i;

	status = validate(req);
	if (status != NI_OK) {
		return (&status);
	}
	/*
	 * Unregister it
	 */
	status = NI_NOTAG;
	for (i = 0; i < regs.regs_len; i++) {
		if (ni_name_match(*tag, regs.regs_val[i].tag)) {
			regs.regs_val[i] = regs.regs_val[regs.regs_len - 1];
			MM_SHRINK_ARRAY(regs.regs_val, regs.regs_len);
			regs.regs_len--;
			status = NI_OK;
			break;
		}
	}
	if (status != NI_OK) {
		return (&status);
	}

	/*
	 * Then kill it
	 */
	for (i = 0; i < nservers; i++) {
		if (ni_name_match(servers[i].name, *tag)) {
			kill(servers[i].pid, SIGKILL);
			ni_name_free(&servers[i].name);
			servers[i] = servers[nservers - 1];
			MM_SHRINK_ARRAY(servers, nservers);
			nservers--;
			break;
		}
	}
	destroydir(*tag);
	return (&status);
	
}

void *
nibind_bind_1(
	      nibind_bind_args *args,
	      struct svc_req *req
	      )
{
	unsigned port;
	ni_index i;
	ni_binding binding;
	struct timeval tv;
	CLIENT *cl;
	struct sockaddr_in sin;
	int sock;

	port = 0;
	for (i = 0; i < regs.regs_len; i++) {
		if (ni_name_match(args->server_tag, regs.regs_val[i].tag)) {
			port = regs.regs_val[i].addrs.udp_port;
			break;
		}
	}
	if (port == 0) {
		return (NULL);
	}
	sin.sin_port = port;
	sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sin.sin_family = AF_INET;
	MM_ZERO(&sin.sin_zero);
	sock = RPC_ANYSOCK;
	tv.tv_sec = BIND_TIMEOUT;
	tv.tv_usec = 0;
	cl = clntudp_create(&sin, NI_PROG, NI_VERS, tv, &sock);
	if (cl == NULL) {
		return (NULL);
	}
	binding.tag = args->client_tag;
	binding.addr = args->client_addr;
	tv.tv_sec /= (BIND_RETRIES + 1);
	if (clnt_call(cl, _NI_BIND, xdr_ni_binding, &binding,
		      xdr_void, NULL, tv) != RPC_SUCCESS) {
		clnt_destroy(cl);
		return (NULL);
	}
	clnt_destroy(cl);
	return ((void *)~0);
}


/*
 * Destroy a database directory
 */
static int
dir_destroy(
	     char *dir
	     )
{
	char path1[MAXPATHLEN + 1];
	char path2[MAXPATHLEN + 1];
	DIR *dp;
	struct direct *d;

	dp = opendir(dir);
	if (dp == NULL) {
		return (0);
	}
	while (d = readdir(dp)) {
		sprintf(path1, "%s/%.*s", dir, d->d_namlen, d->d_name);
		sprintf(path2, "./%.*s.tmp", d->d_namlen, d->d_name);
		/*
		 * rename, then unlink in case NFS leaves tmp files behind
		 * (.nfs* files, that is).
		 */
		if (rename(path1, path2) != 0 ||
		    unlink(path2) != 0) {
			/* ignore error: rmdir will catch ENOTEMPTY */
		}
	}
	closedir(dp);
	return (rmdir(dir));
}

static const char NI_SUFFIX_CUR[] = ".nidb";
static const char NI_SUFFIX_MOVED[] = ".move";
static const char NI_SUFFIX_TMP[] = ".temp";

void
dir_getnames(
	     ni_name orig,
	     ni_name *target,
	     ni_name *moved,
	     ni_name *tmp
	     )
{
	if (target != NULL) {
		*target = malloc(strlen(orig) + strlen(NI_SUFFIX_CUR) + 1);
		sprintf(*target, "%s%s", orig, NI_SUFFIX_CUR);
	}
	if (moved != NULL) {
		*moved = malloc(strlen(orig) + strlen(NI_SUFFIX_MOVED) + 1);
		sprintf(*moved, "%s%s", orig, NI_SUFFIX_MOVED);
	}
	if (tmp != NULL) {
		*tmp = malloc(strlen(orig) + strlen(NI_SUFFIX_TMP) + 1);
		sprintf(*tmp, "%s%s", orig, NI_SUFFIX_TMP);
	}
}
  
void
destroydir(
	   ni_name tag
	   )
{
	ni_name target;
	ni_name moved;
	ni_name tmp;

	dir_getnames(tag, &target, &moved, &tmp);
	dir_destroy(target);
	dir_destroy(moved);
	dir_destroy(tmp);
	ni_name_free(&target);
	ni_name_free(&moved);
	ni_name_free(&tmp);
}


