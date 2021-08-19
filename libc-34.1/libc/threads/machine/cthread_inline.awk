NF == 2 && $1 == "jbsr" && $2 == "_spin_lock" {
	print	"	movel sp@,a0"
	print	"1:	tas a0@"
	print	"	bne 1b"
	continue
}
NF == 2 && $1 == "jbsr" && $2 == "_mutex_try_lock" {
	print	"	movel sp@,a0"
	print	"	tas a0@"
	print	"	beq 1f"
	print	"	clrl d0"
	print	"	jra 2f"
	print	"1:	moveq #1,d0"
	print	"2:"
	continue
}
NF == 2 && $1 == "jbsr" && $2 == "_cthread_sp" {
	print	"	movel sp,d0"
	continue
}
# default:
{
	print
}
