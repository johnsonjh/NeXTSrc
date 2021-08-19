
| Stubs for defaults in libNeXT.

| This code replaces defaults in libNeXT with jumps to the libsys version of defaults.  It
| is only necessary for 1.0 backward compatibility.

	.text

	.globl	__NXRegisterDefaults_old
	.globl	__NXGetDefaultValue_old
	.globl	__NXSetDefault_old
	.globl	__NXWriteDefault_old
	.globl	__NXWriteDefaults_old
	.globl	__NXRemoveDefault_old
	.globl	__NXReadDefault_old
	.globl	__NXUpdateDefaults_old
	.globl	__NXUpdateDefault_old
	.globl	__NXSetDefaultsUser_old
	.globl	__NXFilePathSearch_old
	.globl	__NXGetTempFilename_old

__NXRegisterDefaults_old:
	movel	__libNeXT_NXArgc, 0x0401064c
	movel	__libNeXT_NXArgv, 0x040105f4
	jmp	0x050043fa
__NXGetDefaultValue_old:
	movel	__libNeXT_NXArgc, 0x0401064c
	movel	__libNeXT_NXArgv, 0x040105f4
	jmp	0x05004400
__NXSetDefault_old:
	movel	__libNeXT_NXArgc, 0x0401064c
	movel	__libNeXT_NXArgv, 0x040105f4
	jmp	0x05004406
__NXWriteDefault_old:
	movel	__libNeXT_NXArgc, 0x0401064c
	movel	__libNeXT_NXArgv, 0x040105f4
	jmp	0x0500440c
__NXWriteDefaults_old:
	movel	__libNeXT_NXArgc, 0x0401064c
	movel	__libNeXT_NXArgv, 0x040105f4
	jmp	0x05004412
__NXRemoveDefault_old:
	movel	__libNeXT_NXArgc, 0x0401064c
	movel	__libNeXT_NXArgv, 0x040105f4
	jmp	0x05004418
__NXReadDefault_old:
	movel	__libNeXT_NXArgc, 0x0401064c
	movel	__libNeXT_NXArgv, 0x040105f4
	jmp	0x0500441e
__NXUpdateDefaults_old:
	movel	__libNeXT_NXArgc, 0x0401064c
	movel	__libNeXT_NXArgv, 0x040105f4
	jmp	0x05004424
__NXUpdateDefault_old:
	movel	__libNeXT_NXArgc, 0x0401064c
	movel	__libNeXT_NXArgv, 0x040105f4
	jmp	0x0500442a
__NXSetDefaultsUser_old:
	movel	__libNeXT_NXArgc, 0x0401064c
	movel	__libNeXT_NXArgv, 0x040105f4
	jmp	0x05004430
__NXFilePathSearch_old:
	movel	__libNeXT_NXArgc, 0x0401064c
	movel	__libNeXT_NXArgv, 0x040105f4
	jmp	0x05004436
__NXGetTempFilename_old:
	movel	__libNeXT_NXArgc, 0x0401064c
	movel	__libNeXT_NXArgv, 0x040105f4
	jmp	0x0500443c
	
