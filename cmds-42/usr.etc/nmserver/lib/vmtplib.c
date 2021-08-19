/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * Support for test programs - library to be.
 */

#define	KERNEL_FEATURES

/*#include <sys/options.h>*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
/*#include <netinet/vmtp_sys.h>*/
#include <netinet/vmtp_so.h>
#include <netinet/vmtp.h>
#include <netinet/vmtp_manager.h>



/*
 * The created entity ids aren't guaranteed to be unique. Unique id's are
 * created by alloceid() but this requires a VMTP socket bound to an eid!
 */
struct vmtpeid *createeid()
{
	struct vmtpeid eid;

    	eid.ve_inaddr = ntohl(gethostid());
    	setve_tmstamp(eid, time(0));
#ifdef LITTLE_ENDIAN
    	setve_flags(eid, VE_LEE);
#else
    	setve_flags(eid, 0);
#endif
	return &eid;
}


alloceid(s, eidp)
	int s;
	struct vmtpeid *eidp;
{
	struct vmtpmcb mcb;
	struct vmtpmreq *vm = (struct vmtpmreq *)&mcb.vm_ucb;

	mcb.vm_server.ve_fltm = VMTP_MANAGER_FLTM;
	mcb.vm_server.ve_inaddr = VMTP_HOSTGROUP;
	vm->vmr_code = VM_NEWENT;
	vm->vmr_eid = *eidp;
	if (invoke(s, INVOKE_REQ | INVOKE_RESP, &mcb, (char *)0, 0) < 0)
		mcb.vm_code = -1;
	*eidp = vm->vmr_eid;
	return mcb.vm_code;
	
}

initserver(s, server, flags)
	int s;
	struct vmtpeid server;
	int flags;
{
	struct vmtpmcb mcb;
	struct vmtpmreq *vm = (struct vmtpmreq *)&mcb.vm_ucb;

	mcb.vm_server.ve_fltm = VMTP_MANAGER_FLTM;
	mcb.vm_server.ve_inaddr = VMTP_HOSTGROUP;
	vm->vmr_code = VM_NEWSERVER;
	vm->vmr_eid = server;
	vm->vmr_flags = flags;
	if (invoke(s, INVOKE_REQ | INVOKE_RESP, &mcb, (char *)0, 0) < 0)
		mcb.vm_code = -1;
	return mcb.vm_code;
}

joingroup(s, group)
	int s;
	struct vmtpeid group;
{
	struct vmtpmcb mcb;
	struct vmtpmreq *vm = (struct vmtpmreq *)&mcb.vm_ucb;

	mcb.vm_server.ve_fltm = VMTP_MANAGER_FLTM;
	mcb.vm_server.ve_inaddr = VMTP_HOSTGROUP;
	vm->vmr_code = VM_JOINGROUP;
	vm->vmr_eid.ve_fltm = 0;	/* defaults to invoker */
	vm->vmr_eid.ve_inaddr = 0;
	vm->vmr_group = group;
	if (invoke(s, INVOKE_REQ | INVOKE_RESP, &mcb, (char *)0, 0) < 0)
		mcb.vm_code = -1;
	return mcb.vm_code;

}

leavegroup(s, group)
	int s;
	struct vmtpeid group;
{
	struct vmtpmcb mcb;
	struct vmtpmreq *vm = (struct vmtpmreq *)&mcb.vm_ucb;

	mcb.vm_server.ve_fltm = VMTP_MANAGER_FLTM;
	mcb.vm_server.ve_inaddr = VMTP_HOSTGROUP;
	vm->vmr_code = VM_LEAVEGROUP;
	vm->vmr_eid.ve_fltm = 0;	/* defaults to invoker */
	vm->vmr_eid.ve_inaddr = 0;
	vm->vmr_group = group;
	if (invoke(s, INVOKE_REQ | INVOKE_RESP, &mcb, (char *)0, 0) < 0)
		mcb.vm_code = -1;
	return mcb.vm_code;

}




