/*
 * This file contains the pointers to symbols not defined in libNeXT but used
 * by the code in libNeXT.  For the pointers who's symbols are expected to be
 * in libsys they are initialized to the addresses in libsys (for routines this
 * is the address of a branch table slot not the actual address of the routine).
 *
 * New pointers are to be added at the end of this file before the symbol 
 * _libNeXT_pointers_pad and the pad to be reduced by the number of pointers
 * added.  This keeps the global data size of this file constant and thus does
 * not effect the addresses of other symbols.
 */

/* Defined in crt0.o */
void * _libNeXT_NXArgc = (void *)0x0; /* a data symbol unpredictable address */
void * _libNeXT_NXArgv = (void *)0x0; /* a data symbol unpredictable address */

/* Defined in libsys_s.a */

/* only used in aliases, no macros in shlib.h for these */
void * _libNeXT_objc_msgSend = (void *)0x0500387e;
void * _libNeXT_objc_msgSendSuper = (void *)0x05003884;
void * _libNeXT_objc_getClass = (void *)0x05003866;
void * _libNeXT_objc_getMetaClass = (void *)0x05003872;
void * _libNeXT_bcopy = (void *)0x050021c8;
void * _libNeXT__fixunsdfsi = (void *)0x0500209c;

void * _libNeXT_NXAllocErrorData = (void *)0x05003c68;
void * _libNeXT_unusedNXArchiveBitFields = (void *)0x00000000;
void * _libNeXT_unusedNXArchiveChar = (void *)0x00000000;
void * _libNeXT_unusedNXArchiveDouble = (void *)0x00000000;
void * _libNeXT_unusedNXArchiveFloat = (void *)0x00000000;
void * _libNeXT_unusedNXArchiveInt = (void *)0x00000000;
void * _libNeXT_unusedNXArchiveLong = (void *)0x00000000;
void * _libNeXT_unusedNXArchiveShort = (void *)0x00000000;
void * _libNeXT_NXClose = (void *)0x05003c74;
void * _libNeXT_NXCloseMemory = (void *)0x05003c7a;
void * _libNeXT_NXCloseTypedStream = (void *)0x05003926;
void * _libNeXT_NXDefaultRead = (void *)0x05003c86;
void * _libNeXT_NXDefaultWrite = (void *)0x05003c8c;
void * _libNeXT_NXEndOfTypedStream = (void *)0x0500394a;
void * _libNeXT_NXFlush = (void *)0x05003c98;
void * _libNeXT_NXFlushTypedStream = (void *)0x05003950;
void * _libNeXT_NXFreeObjectBuffer = (void *)0x0500395c;
void * _libNeXT_NXGetMemoryBuffer = (void *)0x05003ca4;
void * _libNeXT_NXMapFile = (void *)0x05003caa;
void * _libNeXT_NXOpenFile = (void *)0x05003cb0;
void * _libNeXT_NXOpenMemory = (void *)0x05003cb6;
void * _libNeXT_NXOpenTypedStream = (void *)0x05003992;
void * _libNeXT_NXOpenTypedStreamForFile = (void *)0x05003998;
void * _libNeXT_NXPrintf = (void *)0x05003cc2;
void * _libNeXT_NXReadArray = (void *)0x050039aa;
void * _libNeXT_NXReadClass = (void *)0x00000000;
void * _libNeXT_NXReadObject = (void *)0x050039b0;
void * _libNeXT_NXReadObjectFromBuffer = (void *)0x050039b6;
void * _libNeXT_NXReadType = (void *)0x050039bc;
void * _libNeXT_NXReadTypes = (void *)0x050039c2;
void * _libNeXT_NXResetErrorData = (void *)0x05003d70;
void * _libNeXT_NXSaveToFile = (void *)0x05003cc8;
void * _libNeXT_NXScanf = (void *)0x05003cce;
void * _libNeXT_NXSeek = (void *)0x05003cd4;
void * _libNeXT_NXStreamCreate = (void *)0x05003ce0;
void * _libNeXT_NXSystemVersion = (void *)0x050039da;
void * _libNeXT_NXTell = (void *)0x05003cec;
void * _libNeXT_unusedNXUnarchiveBitFields = (void *)0x00000000;
void * _libNeXT_unusedNXUnarchiveChar = (void *)0x00000000;
void * _libNeXT_unusedNXUnarchiveDouble = (void *)0x00000000;
void * _libNeXT_unusedNXUnarchiveFloat = (void *)0x00000000;
void * _libNeXT_unusedNXUnarchiveInt = (void *)0x00000000;
void * _libNeXT_unusedNXUnarchiveLong = (void *)0x00000000;
void * _libNeXT_unusedNXUnarchiveShort = (void *)0x00000000;
void * _libNeXT_NXUngetc = (void *)0x05003cf2;
void * _libNeXT_NXVPrintf = (void *)0x05003cf8;
void * _libNeXT_NXWriteArray = (void *)0x05003a22;
void * _libNeXT_NXWriteClass = (void *)0x00000000;
void * _libNeXT_NXWriteObject = (void *)0x05003a28;
void * _libNeXT_NXWriteObjectReference = (void *)0x05003a2e;
void * _libNeXT_NXWriteRootObject = (void *)0x05003a34;
void * _libNeXT_NXWriteRootObjectToBuffer = (void *)0x05003a3a;
void * _libNeXT_NXWriteType = (void *)0x05003a40;
void * _libNeXT_NXWriteTypes = (void *)0x05003a46;
void * _libNeXT_SNDAlloc = (void *)0x05003ab8;
void * _libNeXT_SNDCompactSamples = (void *)0x05003aca;
void * _libNeXT_SNDCopySamples = (void *)0x05003ad6;
void * _libNeXT_SNDCopySound = (void *)0x05003adc;
void * _libNeXT_SNDDeleteSamples = (void *)0x05003ae2;
void * _libNeXT_SNDFree = (void *)0x05003ae8;
void * _libNeXT_SNDInsertSamples = (void *)0x05003b00;
void * _libNeXT_SNDReadSoundfile = (void *)0x05003b2a;
void * _libNeXT_SNDSamplesProcessed = (void *)0x05003b4e;
void * _libNeXT_SNDSampleCount = (void *)0x05003b48;
void * _libNeXT_SNDStartPlaying = (void *)0x05003b72;
void * _libNeXT_SNDStartRecording = (void *)0x05003b78;
void * _libNeXT_SNDStop = (void *)0x05003b7e;
void * _libNeXT_SNDWait = (void *)0x05003b8a;
void * _libNeXT_SNDWriteSoundfile = (void *)0x05003b9c;
void * _libNeXT_SNDiMulaw = (void *)0x05003ba2;
void * _libNeXT__NXAddHandler = (void *)0x05003d0a;
void * _libNeXT__unusedNXArchiveData = (void *)0x00000000;
void * _libNeXT__unusedNXIgnore = (void *)0x00000000;
void * _libNeXT__NXRaiseError = (void *)0x05003d10;
void * _libNeXT__unusedNXReadObject = (void *)0x00000000;
void * _libNeXT__unusedNXReadSelf = (void *)0x00000000;
void * _libNeXT__NXRemoveHandler = (void *)0x05003d1c;
void * _libNeXT__NXStreamChangeBuffer = (void *)0x05003d22;
void * _libNeXT__NXStreamFillBuffer = (void *)0x05003d28;
void * _libNeXT__NXStreamFlushBuffer = (void *)0x05003d2e;
void * _libNeXT__unusedNXUnarchiveData = (void *)0x00000000;
void * _libNeXT__NXUncaughtExceptionHandler = (void *)0x040107c4;
void * _libNeXT__unusedNXWriteObject = (void *)0x00000000;
void * _libNeXT__unusedNXWriteSelf = (void *)0x00000000;
void * _libNeXT__alloc = (void *)0x04010bc0;
void * _libNeXT__ctype_ = (void *)0x050062b0;
void * _libNeXT__cvtToId = (void *)0x04010bd0;
void * _libNeXT__cvtToSel = (void *)0x04010bd4;
void * _libNeXT__error = (void *)0x04010bd8;
void * _libNeXT__filbuf = (void *)0x0500208a;
void * _libNeXT__flsbuf = (void *)0x050020a2;
void * _libNeXT__iob = (void *)0x04010000;
void * _libNeXT__longjmp = (void *)0x050020b4;
void * _libNeXT__setjmp = (void *)0x050020de;
void * _libNeXT__strhash = (void *)0x05003812;
void * _libNeXT_abort = (void *)0x05002126;
void * _libNeXT_access = (void *)0x05002138;
void * _libNeXT_alarm = (void *)0x05002156;
/* this used to be alloca's pointer, a function which we should not be using out of libc.  If we get rid of this substitution, we the compiler will generate correct code inline. */
void * _libNeXT_FORMER_alloca = 0;
void * _libNeXT_atoi = (void *)0x0500219e;
void * _libNeXT_ceil = (void *)0x0500370a;
void * _libNeXT_chdir = (void *)0x05002228;
void * _libNeXT_close = (void *)0x0500229a;
void * _libNeXT_closedir = (void *)0x050022a0;
void * _libNeXT_connect = (void *)0x050022c4;
void * _libNeXT_cthread_detach = (void *)0x05002300;
void * _libNeXT_cthread_fork = (void *)0x0500230c;
void * _libNeXT_cthread_init = (void *)0x05002312;
void * _libNeXT_cthread_name = (void *)0x05002324;
void * _libNeXT_ctime = (void *)0x05002348;
void * _libNeXT_dbClose = (void *)0x05003d88;
void * _libNeXT_dbDelete = (void *)0x05003da0;
void * _libNeXT_dbFetch = (void *)0x05003db8;
void * _libNeXT_dbOpen = (void *)0x05003de8;
void * _libNeXT_dbStore = (void *)0x05003e0c;
void * _libNeXT_dbUnlock = (void *)0x05003e18;
void * _libNeXT_dbWaitLock = (void *)0x05003e1e;
void * _libNeXT_endpwent = (void *)0x050023f0;
void * _libNeXT_errno = (void *)0x040105b0;
void * _libNeXT_execl = (void *)0x0500248c;
void * _libNeXT_exit = (void *)0x050024b0;
void * _libNeXT_fclose = (void *)0x050024d4;
void * _libNeXT_fcntl = (void *)0x050024da;
void * _libNeXT_fflush = (void *)0x050024f8;
void * _libNeXT_fgets = (void *)0x05002510;
void * _libNeXT_floor = (void *)0x05003746;
void * _libNeXT_fopen = (void *)0x0500251c;
void * _libNeXT_fork = (void *)0x05002522;
void * _libNeXT_fprintf = (void *)0x0500252e;
void * _libNeXT_fread = (void *)0x05002540;
void * _libNeXT_free = (void *)0x05002546;
void * _libNeXT_fseek = (void *)0x0500255e;
void * _libNeXT_fstat = (void *)0x0500256a;
void * _libNeXT_ftruncate = (void *)0x05002588;
void * _libNeXT_fwrite = (void *)0x0500258e;
void * _libNeXT_getenv = (void *)0x050025d0;
void * _libNeXT_geteuid = (void *)0x050025d6;
void * _libNeXT_gethostbyname = (void *)0x0500262a;
void * _libNeXT_gethostname = (void *)0x0500263c;
void * _libNeXT_getlogin = (void *)0x05002648;
void * _libNeXT_getpid = (void *)0x05002690;
void * _libNeXT_getpwent = (void *)0x050026ba;
void * _libNeXT_getpwnam = (void *)0x050026c0;
void * _libNeXT_getpwuid = (void *)0x050026c6;
void * _libNeXT_getsectbyname = (void *)0x050026f6;
void * _libNeXT_getsectdata = (void *)0x05002702;
void * _libNeXT_gettimeofday = (void *)0x05002738;
void * _libNeXT_getuid = (void *)0x0500274a;
void * _libNeXT_getwd = (void *)0x0500275c;
void * _libNeXT_link = (void *)0x0500284c;
void * _libNeXT_longjmp = (void *)0x05002864;
void * _libNeXT_lseek = (void *)0x050028ac;
void * _libNeXT_lstat = (void *)0x050028b2;
void * _libNeXT_mach_error_string = (void *)0x050028ca;
void * _libNeXT_mach_ports_lookup = (void *)0x050028e8;
void * _libNeXT_mach_ports_register = (void *)0x050028ee;
void * _libNeXT_malloc = (void *)0x050028fa;
void * _libNeXT_malloc_error = (void *)0x05002906;
void * _libNeXT_map_fd = (void *)0x0500291e;
void * _libNeXT_memmove = (void *)0x0500294e;
void * _libNeXT_memset = (void *)0x05002954;
void * _libNeXT_mig_dealloc_reply_port = (void *)0x0500295a;
void * _libNeXT_mig_get_reply_port = (void *)0x05002960;
void * _libNeXT_mkdir = (void *)0x0500296c;
void * _libNeXT_msg_receive = (void *)0x050029ae;
void * _libNeXT_msg_rpc = (void *)0x050029c0;
void * _libNeXT_msg_send = (void *)0x050029d2;
void * _libNeXT_mutex_try_lock = (void *)0x050029ea;
void * _libNeXT_mutex_wait_lock = (void *)0x050029f6;
void * _libNeXT_name_server_port = (void *)0x04010294;
void * _libNeXT_netname_check_in = (void *)0x05002a08;
void * _libNeXT_netname_check_out = (void *)0x05002a0e;
void * _libNeXT_netname_look_up = (void *)0x05002a14;
void * _libNeXT_open = (void *)0x05002bc4;
void * _libNeXT_opendir = (void *)0x05002bca;
void * _libNeXT_pclose = (void *)0x05002c1e;
void * _libNeXT_pipe = (void *)0x05002c2a;
void * _libNeXT_popen = (void *)0x05002c4e;
void * _libNeXT_port_allocate = (void *)0x05002c54;
void * _libNeXT_port_deallocate = (void *)0x05002c5a;
void * _libNeXT_port_set_add = (void *)0x05002c90;
void * _libNeXT_port_set_allocate = (void *)0x05002c96;
void * _libNeXT_port_set_backlog = (void *)0x05002c9c;
void * _libNeXT_port_set_deallocate = (void *)0x05002ca8;
void * _libNeXT_port_set_remove = (void *)0x05002cae;
void * _libNeXT_port_status = (void *)0x05002cc0;
void * _libNeXT_pow = (void *)0x05003788;
void * _libNeXT_prdb_end = (void *)0x05002ccc;
void * _libNeXT_prdb_get = (void *)0x05002cd2;
void * _libNeXT_prdb_getbyname = (void *)0x05002cd8;
void * _libNeXT_prdb_set = (void *)0x05002cde;
void * _libNeXT_printf = (void *)0x05002ce4;
void * _libNeXT_read = (void *)0x05002d62;
void * _libNeXT_readdir = (void *)0x05002d68;
void * _libNeXT_realloc = (void *)0x05002d7a;
void * _libNeXT_seekdir = (void *)0x05002e34;
void * _libNeXT_select = (void *)0x05002e3a;
void * _libNeXT_setpwent = (void *)0x05002f06;
void * _libNeXT_signal = (void *)0x05002f7e;
void * _libNeXT_sigvec = (void *)0x05002f9c;
void * _libNeXT_socket = (void *)0x05002fb4;
void * _libNeXT_sprintf = (void *)0x05002fcc;
void * _libNeXT_sscanf = (void *)0x05002fde;
void * _libNeXT_stat = (void *)0x05002fea;
void * _libNeXT_strcat = (void *)0x05002ff6;
void * _libNeXT_strchr = (void *)0x05003002;
void * _libNeXT_strcmp = (void *)0x05003008;
void * _libNeXT_strcpy = (void *)0x0500301a;
void * _libNeXT_strlen = (void *)0x05003038;
void * _libNeXT_strncat = (void *)0x0500303e;
void * _libNeXT_strncmp = (void *)0x05003044;
void * _libNeXT_strncpy = (void *)0x0500304a;
void * _libNeXT_strrchr = (void *)0x05003056;
void * _libNeXT_strtod = (void *)0x05003068;
void * _libNeXT_strtok = (void *)0x0500306e;
void * _libNeXT_system = (void *)0x05003128;
void * _libNeXT_task_self_ = (void *)0x04010290;
void * _libNeXT_task_set_special_port = (void *)0x05003164;
void * _libNeXT_thread_abort = (void *)0x05003188;
void * _libNeXT_time = (void *)0x050031d0;
void * _libNeXT_umask = (void *)0x0500323c;
void * _libNeXT_unlink = (void *)0x0500324e;
void * _libNeXT_vfork = (void *)0x0500328a;
void * _libNeXT_vfprintf = (void *)0x05003290;
void * _libNeXT_vm_allocate = (void *)0x050032a8;
void * _libNeXT_vm_deallocate = (void *)0x050032ba;
void * _libNeXT_wait = (void *)0x050032f6;
void * _libNeXT_wait3 = (void *)0x050032fc;
void * _libNeXT_write = (void *)0x0500330e;
void * _libNeXT_unusedNXSYSTEMVERSION = (void *)0x00000000;

void * _libNeXT_sel_isMapped = (void *)0x050038ba;
void * _libNeXT_sel_getName = (void *)0x050038ae;
void * _libNeXT_sel_getUid = (void *)0x050038b4;
void * _libNeXT_object_getClassName = (void *)0x05003896;

void * _libNeXT_NXCreateHashTable = (void *)0x0500393e;
void * _libNeXT_NXFreeHashTable = (void *)0x05003956;
void * _libNeXT_NXEmptyHashTable = (void *)0x05003944;
void * _libNeXT_NXCopyHashTable = (void *)0x0500392c;
void * _libNeXT_NXCountHashTable = (void *)0x05003938;
void * _libNeXT_NXHashMember = (void *)0x05003974;
void * _libNeXT_NXHashGet = (void *)0x05003962;
void * _libNeXT_NXHashInsert = (void *)0x05003968;
void * _libNeXT_NXHashRemove = (void *)0x0500397a;
void * _libNeXT_NXInitHashState = (void *)0x05003980;
void * _libNeXT_NXNextHashState = (void *)0x05003986;
void * _libNeXT_NXPtrHash = (void *)0x0500399e;
void * _libNeXT_NXStrHash = (void *)0x050039ce;
void * _libNeXT_NXPtrIsEqual = (void *)0x050039a4;
void * _libNeXT_NXStrIsEqual = (void *)0x050039d4;
void * _libNeXT_NXNoEffectFree = (void *)0x0500398c;
void * _libNeXT_NXReallyFree = (void *)0x050039c8;
void * _libNeXT_NXUniqueString = (void *)0x05003a10;
void * _libNeXT_NXUniqueStringWithLength = (void *)0x05003a1c;
void * _libNeXT_NXCopyStringBuffer = (void *)0x05003932;
void * _libNeXT_qsort = (void *)0x05002d1a;
void * _libNeXT__NXAddAltHandler = (void *)0x05003d04;
void * _libNeXT__NXRemoveAltHandler = (void *)0x05003d16;
void * _libNeXT_NXTypedStreamClassVersion = (void *)0x050039e0;
void * _libNeXT_vsprintf = (void *)0x050032ea;

void * _libNeXT_sleep = (void *)0x05002fa2;
void * _libNeXT_calloc = (void *)0x0500220a;
void * _libNeXT_kern_timestamp = (void *)0x05002828;

void * _libNeXT_SNDBytesToSamples = (void *)0x05003ac4;
void * _libNeXT_SNDConvertSound = (void *)0x05003ad0;
void * _libNeXT_SNDGetVolume = (void *)0x05003afa;
void * _libNeXT_SNDGetMute = (void *)0x05003af4;
void * _libNeXT_SNDMulaw = (void *)0x05003b0c;
void * _libNeXT_SNDSetMute = (void *)0x05003b60;
void * _libNeXT_SNDSetVolume = (void *)0x05003b66;

void * _libNeXT_getsectdatafromlib = (void *)0x0500270e;
void * _libNeXT_openlog = (void *)0x05002bd0;
void * _libNeXT_syslog = (void *)0x05003122;
void * _libNeXT_fputc = (void *)0x05002534;

void * _libNeXT_memcmp = (void *)0x05002942;

void * _libNeXT_ioctl = (void *)0x050027ce;

void * _libNeXT_getgid = (void *)0x05002606;
void * _libNeXT_getegid = (void *)0x050025ca;
void * _libNeXT_chown = (void *)0x05002234;
void * _libNeXT_dbExists = (void *)0x05003dac;
void * _libNeXT_dbCreate = (void *)0x05003d94;
void * _libNeXT_dbInit = (void *)0x05003dd6;

void * _libNeXT_malloc_debug = (void *)0x05002900;

void * _libNeXT_mach_error = (void *)0x050028c4;

void * _libNeXT_scandir = (void *)0x05002e28;
void * _libNeXT_alphasort = (void *)0x05002180;
void * _libNeXT_getsectdatafromheader = (void *)0x05002708;
void * _libNeXT_getsectbynamefromheader = (void *)0x050026fc;

void * _libNeXT_cthread_set_name = (void *)0x05002330;

void * _libNeXT_vm_page_size = (void *)0x040102a0;
void * _libNeXT_getdtablesize = (void *)0x050025c4;
void * _libNeXT_execv = (void *)0x0500249e;

void * _libNeXT_snddriver_stream_start_reading = (void *)0x05003c26;
void * _libNeXT_snddriver_stream_setup = (void *)0x05003c20;
void * _libNeXT_snddriver_reply_handler = (void *)0x05003bf0;
void * _libNeXT_snddriver_stream_control = (void *)0x05003c14;
void * _libNeXT_snddriver_set_sndin_owner_port = (void *)0x05003c02;

void * _libNeXT_vm_copy = (void *)0x050032b4;
void * _libNeXT_vm_protect = (void *)0x050032c6;

void * _libNeXT_NXUniqueStringNoCopy = (void *)0x05003a16;

void * _libNeXT_valloc = (void *)0x05003284;
void * _libNeXT_NXStrPrototype = (void *)0x050067c4;

void * _libNeXT_SNDBootDSP = (void *)0x05003abe;
void * _libNeXT_snddriver_dsp_write = (void *)0x05003bc6;
void * _libNeXT_SNDReadDSPfile = (void *)0x05003b1e;
void * _libNeXT_snddriver_set_dsp_owner_port = (void *)0x05003bfc;
void * _libNeXT_snddriver_get_dsp_cmd_port = (void *)0x05003be4;
void * _libNeXT_snddriver_dsp_protocol = (void *)0x05003bb4;

void * _libNeXT_abs = (void *)0x0500212c;
void * _libNeXT_moncontrol = (void *)0x050029a2;

void * _libNeXT_getdirentries = (void *)0x050025ac;
void * _libNeXT_mstats = (void *)0x050029e4;

void * _libNeXT_atan = (void *)0x050036ec;
void * _libNeXT_cos = (void *)0x05003716;
void * _libNeXT_sin = (void *)0x0500379a;
void * _libNeXT_sqrt = (void *)0x050037a6;
void * _libNeXT_fabs = (void *)0x050024c2;
void * _libNeXT__mh_execute_header = (void *)0x2000;
void * _libNeXT_object_setInstanceVariable = (void *)0x050038a8;

void * _libNeXT_thread_info = (void *)0x050031a0;
void * _libNeXT_thread_policy = (void *)0x05004100;
void * _libNeXT_thread_priority = (void *)0x050040e8;
void * _libNeXT_thread_self = (void *)0x050031b2;
void * _libNeXT_getrusage = (void *)0x050026ea;

void * _libNeXT_SNDStartRecordingFile = (void *)0x050042c2;

void * _libNeXT__fixdfsi = (void *)0x0500426e;

void * _libNeXT_NXCreateHashTableFromZone = (void *)0x050039f8;
void * _libNeXT_NXCopyStringBufferFromZone = (void *)0x050039f2;
void * _libNeXT_NXSetTypedStreamZone = (void *)0x050039fe;
void * _libNeXT_NXCreateZone = (void *)0x0500240e;
void * _libNeXT_NXCreateChildZone = (void *)0x05002414;
void * _libNeXT_NXMergeZone = (void *)0x0500241a;
void * _libNeXT_NXZoneCalloc = (void *)0x05002420;
void * _libNeXT_NXZoneFromPtr = (void *)0x05002426;
void * _libNeXT_NXZonePtrInfo = (void *)0x0500242c;
void * _libNeXT_NXDefaultMallocZone = (void *)0x05002444;
void * _libNeXT_NXStreamCreateFromZone = (void *)0x05003d3a;
void * _libNeXT__zoneAlloc = (void *)0x04010bdc;

void * _libNeXT_task_notify = (void *)0x05003152;
void * _libNeXT_setlocale = (void *)0x05002ed6;
void * _libNeXT_usleep = (void *)0x05003260;
void * _libNeXT_condition_wait = (void *)0x050022be;
void * _libNeXT_cond_signal = (void *)0x050022b8;

void * _libNeXT_NXRegisterDefaults = (void *)0x050043fa;
void * _libNeXT_NXGetDefaultValue = (void *)0x05004400;
void * _libNeXT_NXSetDefault = (void *)0x05004406;
void * _libNeXT_NXWriteDefault = (void *)0x0500440c;
void * _libNeXT_NXWriteDefaults = (void *)0x05004412;
void * _libNeXT_NXRemoveDefault = (void *)0x05004418;
void * _libNeXT_NXReadDefault = (void *)0x0500441e;
void * _libNeXT_NXUpdateDefaults = (void *)0x05004424;
void * _libNeXT_NXUpdateDefault = (void *)0x0500442a;
void * _libNeXT_NXSetDefaultsUser = (void *)0x05004430;
void * _libNeXT_NXFilePathSearch = (void *)0x05004436;
void * _libNeXT_NXGetTempFilename = (void *)0x0500443c;

void * _libNeXT_NXNameZone = (void *)0x0500245c;

void * _libNeXT_bootstrap_look_up = (void *)0x0500416c;
void * _libNeXT_task_get_special_port = (void *)0x05003146;

void * _libNeXT_NXToLower = (void *)0x050044c0;
void * _libNeXT_NXIsLower = (void *)0x05004496;
void * _libNeXT_NXIsDigit = (void *)0x0500448a;
void * _libNeXT_NXToAscii = (void *)0x050044ba;

/*
 * New pointers are added before this symbol.  This must remain at the end of
 * this file.  When a new pointers are added the pad must be reduced by the
 * number of new pointers added.  This symbol must be global and NOT static
 * because it is padding the size of global data in this file so that the size
 * is a constant.  If it were to be static ld(1) would not allow it to be
 * loaded into a shlib because the data would be out of order (static data
 * following global data) and since ld(1) splits global and static data when
 * loaded into a shlib this wouldn't pad the global data.
 */

void *_libNeXT_pointers_pad[144] = { 0 };
