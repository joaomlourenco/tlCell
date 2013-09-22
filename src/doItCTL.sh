#starting point ~/1.0.2/
echo "moving into /1.0.2/src/"
cd src
echo "making .libs directory"
mkdir .libs

#../libtool --tag=CC   --mode=compile ppu-gcc -DHAVE_CONFIG_H -I. -I..     -Wall -Wshadow -Wwrite-strings -Winline 
#-funit-at-a-time --param inline-unit-growth=155 -DLIB_COMPILATION -O2 -fno-strict-aliasing -MT tl.lo -MD -MP -MF .deps/tl.Tpo -c 
#-o tl.lo tl.c

echo "step 1 -> Equivalente to libtool mode=compile"
ppu-gcc -DHAVE_CONFIG_H -I. -I.. -Wall -Wshadow -Wwrite-strings -Winline -funit-at-a-time --param inline-unit-growth=155 -DLIB_COMPILATION -O2 -fno-strict-aliasing -MT tl.lo -MD -MP -MF .deps/tl.Tpo -c tl.c  -fPIC -DPIC -o .libs/tl.o

ppu-gcc -DHAVE_CONFIG_H -I. -I.. -Wall -Wshadow -Wwrite-strings -Winline -funit-at-a-time --param inline-unit-growth=155 -DLIB_COMPILATION -O2 -fno-strict-aliasing -MT tl.lo -MD -MP -MF .deps/tl.Tpo -c tl.c -o tl.o >/dev/null 2>&1
mv -f .deps/tl.Tpo .deps/tl.Plo

echo "step 2 ->Equivalent to libtool mode=link"
ppu-gcc -shared  .libs/tl.o  -lpthread -lspe2  -Wl,-soname -Wl,libctl-cmt-word-64.so.0 -o .libs/libctl-cmt-word-64.so.0.0.0

(cd .libs && rm -f libctl-cmt-word-64.so.0 && ln -s libctl-cmt-word-64.so.0.0.0 libctl-cmt-word-64.so.0)
(cd .libs && rm -f libctl-cmt-word-64.so && ln -s libctl-cmt-word-64.so.0.0.0 libctl-cmt-word-64.so)
ppu-ar cru .libs/libctl-cmt-word-64.a  tl.o
ppu-ranlib .libs/libctl-cmt-word-64.a
(cd .libs && rm -f libctl-cmt-word-64.la && ln -s ../libctl-cmt-word-64.la libctl-cmt-word-64.la)

echo "step 3 -> Installing Libraries"
test -z "/home/lopeici/CTL/CTLlib/lib" || /bin/mkdir -p "/home/lopeici/CTL/CTLlib/lib"
/usr/bin/install -c .libs/libctl-cmt-word-64.so.0.0.0 /home/lopeici/CTL/CTLlib/lib/libctl-cmt-word-64.so.0.0.0
(cd /home/lopeici/CTL/CTLlib/lib && { ln -s -f libctl-cmt-word-64.so.0.0.0 libctl-cmt-word-64.so.0 || { rm -f libctl-cmt-word-64.so.0 && ln -s libctl-cmt-word-64.so.0.0.0 libctl-cmt-word-64.so.0; }; })
(cd /home/lopeici/CTL/CTLlib/lib && { ln -s -f libctl-cmt-word-64.so.0.0.0 libctl-cmt-word-64.so || { rm -f libctl-cmt-word-64.so && ln -s libctl-cmt-word-64.so.0.0.0 libctl-cmt-word-64.so; }; })

/usr/bin/install -c /home/lopeici/1.0.2/src/.libs/libctl-cmt-word-64.lai /home/lopeici/CTL/CTLlib/lib/libctl-cmt-word-64.la
/usr/bin/install -c .libs/libctl-cmt-word-64.a /home/lopeici/CTL/CTLlib/lib/libctl-cmt-word-64.a
chmod 644 /home/lopeici/CTL/CTLlib/lib/libctl-cmt-word-64.a
ppu-ranlib /home/lopeici/CTL/CTLlib/lib/libctl-cmt-word-64.a

echo "entering dark zone.. ldconfig"
PATH="$PATH:/sbin" ldconfig -n /home/lopeici/CTL/CTLlib/lib

echo "Moving header files to specific location"
test -z "/home/lopeici/CTL/CTLlib/include" || /bin/mkdir -p "/home/lopeici/CTL/CTLlib/include"
 /usr/bin/install -c -m 644 'tl.h' '/home/lopeici/CTL/CTLlib/include/tl.h'
 /usr/bin/install -c -m 644 'trace_log.h' '/home/lopeici/CTL/CTLlib/include/trace_log.h'
 /usr/bin/install -c -m 644 'tl_handler.h' '/home/lopeici/CTL/CTLlib/include/tl_handler.h'
 /usr/bin/install -c -m 644 'user_trace_log.h' '/home/lopeici/CTL/CTLlib/include/user_trace_log.h'
 /usr/bin/install -c -m 644 'x86_sync_defns.h' '/home/lopeici/CTL/CTLlib/include/x86_sync_defns.h'

#echo "compiling step 1"
#ppu-gcc -DHAVE_CONFIG_H -I. -I.. -Wall -Wshadow -Wwrite-strings -Winline -funit-at-a-time --param inline-unit-growth=155 
#-DLIB_COMPILATION -O2 -fno-strict-aliasing -MT tl.lo -MD -MP -MF .deps/tl.Tpo -c tl.c -fPIC -DPIC -o .libs/tl.o

#echo "compiling step 2"
#ppu-gcc -DHAVE_CONFIG_H -I. -I.. -Wall -Wshadow -Wwrite-strings -Winline -funit-at-a-time --param inline-unit-growth=155 
-DLIB_COMPILATION -O2 -fno-strict-aliasing -MT tl.lo -MD -MP -MF .deps/tl.Tpo -c tl.c -o tl.o >/dev/null 2>&1

#echo "step 3 -> mv tl.Tpo"
#mv -f .deps/tl.Tpo .deps/tl.Plo

#echo "libtool step 4"
#../libtool --tag=CC   --mode=link ppu-gcc  -Wall -Wshadow -Wwrite-strings -Winline -funit-at-a-time --param 
#inline-unit-growth=155 -DLIB_COMPILATION -O2 -fno-strict-aliasing  -O2 -fno-strict-aliasing -o libctl-cmt-word-64.la -rpath 
#/home/lopeici/CTL/CTLlib/lib tl.lo  -lpthread -lspe2

#echo "step 5 making shared lib"
#ppu-gcc -shared  .libs/tl.o  -lpthread -lspe2  -Wl,-soname -Wl,libctl-cmt-word-64.so.0 -o .libs/libctl-cmt-word-64.so.0.0.0

#echo "step  6"
#(cd .libs && rm -f libctl-cmt-word-64.so.0 && ln -s libctl-cmt-word-64.so.0.0.0 libctl-cmt-word-64.so.0)
#(cd .libs && rm -f libctl-cmt-word-64.so && ln -s libctl-cmt-word-64.so.0.0.0 libctl-cmt-word-64.so)

#echo "step 7"
#ppu-ar cru .libs/libctl-cmt-word-64.a  tl.o
#ppu-ranlib .libs/libctl-cmt-word-64.a

#echo "step 8"
#(cd .libs && rm -f libctl-cmt-word-64.la && ln -s ../libctl-cmt-word-64.la libctl-cmt-word-64.la)

#echo "step 9"
#../libtool   --mode=install /usr/bin/install -c  'libctl-cmt-word-64.la' 
#'/home/lopeici/CTL/CTLlib/lib/libctl-cmt-word-64.la'

#echo "step 9"
#cd ..
#test -z "/home/lopeici/CTL/CTLlib/lib" || /bin/mkdir -p "/home/lopeici/CTL/CTLlib/lib"

#echo "step 10"
#/usr/bin/install -c .libs/libctl-cmt-word-64.so.0.0.0 /home/lopeici/CTL/CTLlib/lib/libctl-cmt-word-64.so.0.0.0
#(cd /home/lopeici/CTL/CTLlib/lib && { ln -s -f libctl-cmt-word-64.so.0.0.0 libctl-cmt-word-64.so.0 || { rm -f 
#libctl-cmt-word-64.so.0 && ln -s libctl-cmt-word-64.so.0.0.0 libctl-cmt-word-64.so.0; }; })
#(cd /home/lopeici/CTL/CTLlib/lib && { ln -s -f libctl-cmt-word-64.so.0.0.0 libctl-cmt-word-64.so || { rm -f 
#libctl-cmt-word-64.so && ln -s libctl-cmt-word-64.so.0.0.0 libctl-cmt-word-64.so; }; })

#echo "step 11"
#/usr/bin/install -c .libs/libctl-cmt-word-64.lai /home/lopeici/CTL/CTLlib/lib/libctl-cmt-word-64.la
#/usr/bin/install -c .libs/libctl-cmt-word-64.a /home/lopeici/CTL/CTLlib/lib/libctl-cmt-word-64.a
#chmod 644 /home/lopeici/CTL/CTLlib/lib/libctl-cmt-word-64.a
#ppu-ranlib /home/lopeici/CTL/CTLlib/lib/libctl-cmt-word-64.a
#PATH="$PATH:/sbin" ldconfig -n /home/lopeici/CTL/CTLlib/lib

#/usr/bin/install -c -m 644 'tl.h' '/home/lopeici/CTL/CTLlib/include/tl.h'
#/usr/bin/install -c -m 644 'trace_log.h' '/home/lopeici/CTL/CTLlib/include/trace_log.h'
#/usr/bin/install -c -m 644 'tl_handler.h' '/home/lopeici/CTL/CTLlib/include/tl_handler.h'
#/usr/bin/install -c -m 644 'user_trace_log.h' '/home/lopeici/CTL/CTLlib/include/user_trace_log.h'
#/usr/bin/install -c -m 644 'x86_sync_defns.h' '/home/lopeici/CTL/CTLlib/include/x86_sync_defns.h'
#../scripts/gen_header.sh ../config.h /home/lopeici/CTL/CTLlib/include




