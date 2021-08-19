
if test ! -d bin; then
	mkdir bin
fi

rm -f bin/dirname
cat > bin/dirname << EOF
#! /bin/csh -f
echo hello
EOF
chmod 755 bin/dirname

cwd="`pwd`"
PATH="$cwd/bin:$cwd:$PATH"; 
echo about to set path
export PATH

echo before...
root="`bin/dirname`"
echo after...
