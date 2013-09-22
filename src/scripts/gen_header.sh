#! /bin/bash
#
# This script generate the wrapper header:
#
# gen_header.sh <config.h> <output_dir>

mode=""
granularity=""
target_cpu_bits=""

set_mode()
{
    if egrep '^#define[ \t]*MODE_ENC([ \t]|$)' $config_path > /dev/null; then
	mode="enc"
    else
	if egrep '^#define[ \t]*MODE_CMT([ \t]|$)' $config_path > /dev/null; then
	    mode="cmt"
	else
	    echo "Couldn't detect mode"
	    exit -1
	fi
    fi
}

set_granularity()
{
    if egrep '^#define[ \t]*OBJ_STM_PO([ \t]|$)' $config_path > /dev/null; then
	granularity="objpo"
    else
	if egrep '^#define[ \t]*OBJ_STM([ \t]|$)' $config_path > /dev/null; then
	    granularity="obj"
	else
	    granularity="word"	    
	fi
    fi
}

set_target_cpu_bits()
{
    if egrep '^#define[ \t]*CPU_X86_64([ \t]|$)' $config_path > /dev/null; then
	target_cpu_bits="64"
    else
	if egrep '^#define[ \t]*CPU_X86([ \t]|$)' $config_path > /dev/null; then
	    target_cpu_bits="32"
	else
		target_cpu_bits="64"
	    #echo "Couldn't target cpu"
	    #exit -1
	fi
    fi
}

if [ $# -ne 2 ]; then
    echo "Sintaxe: $0 <config.h> <output_dir>"
    exit -1
fi

config_path=$1
output_dir=$2

set_mode
set_granularity
set_target_cpu_bits

output_filename="tl_${mode}_${granularity}_${target_cpu_bits}.h"
output_path="${output_dir}/${output_filename}"

# echo "output path: $output_path"

header_macro=`tr 'a-z.' 'A-Z_' <<< "${output_filename}_"`

echo "#ifndef $header_macro" > "$output_path"
echo "#define $header_macro" >> "$output_path"
echo >> "$output_path"

echo '/* These are the macros you should define by hand if you' \
    >> "$output_path"
echo ' * want to #include <tl.h> manually.' >> "$output_path"
echo ' */' >> "$output_path"

echo >> "$output_path"

egrep '[ \t]ENABLE_TRACE([ \t]|$)' $config_path >> "$output_path"
egrep '[ \t]MODE_(CMT|ENC)([ \t]|$)' $config_path >> "$output_path"
egrep '[ \t]USE_TX_HANDLER([ \t]|$)' $config_path >> "$output_path"
egrep '[ \t]CPU_X86(_64)?([ \t]|$)' $config_path >> "$output_path"
egrep '[ \t]SUPPORT_SSE([ \t]|$)' $config_path >> "$output_path"
egrep '[ \t]OBJ_STM(_PO)?([ \t]|$)' $config_path >> "$output_path"

echo >> "$output_path"
echo '#include <tl.h>' >> "$output_path"
echo >> "$output_path"
echo "#endif" >> "$output_path"
