#!/bin/bash

#    (c) 2018-2026 kaalam.ai (The Authors of Jazz)
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.


mode='RUN'    # Set mode='DEBUG' for echoing all variables before confirmation.

if [[ "$1" == "--debug" ]]; then
    mode='DEBUG'
fi


# Section 1. Gathering all necessary information. 1. Fail if necessary. 2. Do nothing without confirmation
# ----------

# 1.1 Help on usage

if [[ $1 = '--help' ]]; then
  cat _config_/help_on_config.txt
  exit 0
fi

printf "(usage: ./config.sh [--debug] [--help])\n\nGathering information ...\n\n"

# 1.2 Get Jazz environment variables

jazz_pwd=$(pwd)

jazz_version=$(cat _config_/version)
jz_processor=$(uname -p)
jazz_distro1=$(cat /etc/*-release | grep '^DISTRIB_ID=' | sed 's/^DISTRIB_ID=//')
jazz_distro2=$(cat /etc/*-release | grep '^DISTRIB_RELEASE=' | sed 's/^DISTRIB_RELEASE=//')

jazz_years=$(date +%Y)
if [ "$jazz_years" != '2017' ]; then jazz_years="2017-$jazz_years"; fi

# 1.3 Link to the MicroHTTPD library

if [ -e '_config_/mhd_include_path' ]; then mhd_inclpath=$(cat _config_/mhd_include_path); else mhd_inclpath='/usr/include'; fi

if [ ! -e "$mhd_inclpath/microhttpd.h" ]; then
  echo "** File $mhd_inclpath/microhttpd.h was not found. **"
  cat _config_/help_mhd_not_found.txt
  exit
fi

if [ -e '_config_/mhd_library_path' ]; then mhd_libpath=$(cat _config_/mhd_library_path)
else
  if [ -e '/usr/lib/x86_64-linux-gnu/libmicrohttpd.so' ]; then mhd_libpath='/usr/lib/x86_64-linux-gnu'; else mhd_libpath='/usr/lib'; fi
fi

if [ ! -e "$mhd_libpath/libmicrohttpd.so" ]; then
  echo "** File $mhd_libpath/libmicrohttpd.so was not found. **"
  cat _config_/help_mhd_not_found.txt
  exit 1
fi

# 1.4 Link to the CURL library

if [ -e '_config_/curl_include_path' ]; then curl_inclpath=$(cat _config_/curl_include_path); else curl_inclpath='/usr/include/x86_64-linux-gnu'; fi

if [ ! -e "$curl_inclpath/curl/curl.h" ]; then
  echo "** File $curl_inclpath/curl/curl.h was not found. **"
  cat _config_/help_curl_not_found.txt
  exit
fi

if [ -e '_config_/curl_library_path' ]; then curl_libpath=$(cat _config_/curl_library_path)
else
  if [ -e '/usr/lib/x86_64-linux-gnu/libcurl.so' ]; then curl_libpath='/usr/lib/x86_64-linux-gnu'; else curl_libpath='/usr/lib'; fi
fi

if [ ! -e "$curl_libpath/libcurl.so" ]; then
  echo "** File $curl_libpath/libcurl.so was not found. **"
  cat _config_/help_curl_not_found.txt
  exit 1
fi

# 1.5 Link to the ZeroMQ library

if [ -e '_config_/zmq_include_path' ]; then zmq_inclpath=$(cat _config_/zmq_include_path); else zmq_inclpath='/usr/local/include'; fi

if [ ! -e "$zmq_inclpath/zmq.h" ]; then
  echo "** File $zmq_inclpath/zmq.h was not found. **"
  cat _config_/help_zmq_not_found.txt
  exit
fi

if [ -e '_config_/zmq_library_path' ]; then zmq_libpath=$(cat _config_/zmq_library_path)
else
  if [ -e '/usr/local/lib/libzmq.so' ]; then zmq_libpath='/usr/local/lib'; else zmq_libpath='/usr/lib/x86_64-linux-gnu'; fi
fi

if [ ! -e "$zmq_libpath/libzmq.so" ]; then
  echo "** File $zmq_libpath/libzmq.so was not found. **"
  cat _config_/help_zmq_not_found.txt
  exit 1
fi

# 1.6 Link to onnxruntime and proto library

if [ -e '_config_/onnx_rt_include_path' ]; then onnx_rt_inclpath=$(cat _config_/onnx_rt_include_path); else onnx_rt_inclpath='/usr/local/include'; fi

if [ ! -e "$onnx_rt_inclpath/onnxruntime_c_api.h" ]; then
  echo "** File $onnx_rt_inclpath/onnxruntime_c_api.h was not found. **"
  cat _config_/help_onnx_not_found.txt
  exit
fi

if [ -e '_config_/onnx_rt_library_path' ]; then onnx_rt_libpath=$(cat _config_/onnx_rt_library_path)
else
  if [ -e '/usr/local/lib/libonnxruntime.so' ]; then onnx_rt_libpath='/usr/local/lib'; else onnx_rt_libpath='/usr/lib/x86_64-linux-gnu'; fi
fi

if [ ! -e "$onnx_rt_libpath/libonnxruntime.so" ]; then
  echo "** File $onnx_rt_libpath/libonnxruntime.so was not found. **"
  cat _config_/help_onnx_not_found.txt
  exit 1
fi

if [ -e '_config_/onnx_proto_include_path' ]; then onnx_proto_inclpath=$(cat _config_/onnx_proto_include_path); else onnx_proto_inclpath='/usr/include'; fi

if [ ! -e "$onnx_proto_inclpath/google/protobuf/port_def.inc" ]; then
  echo "** File $onnx_proto_inclpath/google/protobuf/port_def.inc was not found. **"
  cat _config_/help_onnx_not_found.txt
  exit
fi


# 1.7 Get all the Uplifted modules

cd server || return 1


get_descendant_name ( )
{
  parent_class=$1
  module_path=$2

  if [[ ! -d $module_path ]]
  then
    echo $parent_class
    return 0
  fi

  dep=$(ls $module_path*.h)
  rex="^class *([[:alpha:]_]+).*public.*$parent_class.*$"

  for dp in $dep; do
    if [ -f "$dp" ]; then
      while read ln; do
        if [[ $ln =~ $rex ]]
        then
          echo ${BASH_REMATCH[1]}
          return 0
        fi
      done < $dp
    fi
  done
}

uplifted_bop_parent='Bebop'
uplifted_mod_parent='ModelsAPI'
uplifted_api_parent='API'

uplifted_bop_source='../uplifts/bebop/'
uplifted_mod_source='../uplifts/models_api/'
uplifted_api_source='../uplifts/api/'

uplifted_bop=$(get_descendant_name $uplifted_bop_parent $uplifted_bop_source)
uplifted_mod=$(get_descendant_name $uplifted_mod_parent $uplifted_mod_source)
uplifted_api=$(get_descendant_name $uplifted_api_parent $uplifted_api_source)

testp=$(echo src/*/*/ | sed 's/\ /\n/g' | grep "jazz_.*/tests/$" | tr '\n' ' ')
vpath=$(echo src/*/ "$testp")
jzpat=$(echo "$vpath" | sed 's/\ /\n/g' | grep jazz | tr '\n' ' ')

cpps=$(find src/ | grep '.*jazz\(01\)\?_.*cpp$' | tr '\n' ' ')

if [ "$uplifted_bop" != "$uplifted_bop_parent" ]; then
    vpath+="$uplifted_bop_source "
    jzpat+="$uplifted_bop_source "
    cpps+="$(ls $uplifted_bop_source*.cpp) "
fi

if [ "$uplifted_mod" != "$uplifted_mod_parent" ]; then
    vpath+="$uplifted_mod_source "
    jzpat+="$uplifted_mod_source "
    cpps+="$(ls $uplifted_mod_source*.cpp) "
fi

if [ "$uplifted_api" != "$uplifted_api_parent" ]; then
    vpath+="$uplifted_api_source "
    jzpat+="$uplifted_api_source "
    cpps+="$(ls $uplifted_api_source*.cpp) "
fi

# 1.8 Get all the dependencies

objs=$(echo "$cpps" | sed 's/\ /\n/g' | sed 's/.*\/\(.*cpp\)$/\1/' | sed 's/cpp/o/' | tr '\n' ' ')


recursive_parse_header ( )
{
  dep=$(grep -rnw "$1" -e '^#include[ ]*\"src.*\/\(.*h\|test_.*ctest\)' | sed 's/.*\(src.*h\|src.*ctest\).*/\1/')

  for dp in $dep; do
    if [ -e "$dp" ]; then
      # shellcheck disable=SC2001
      short_name=$(echo "$dp" | sed 's/.*\/\(.*h\|test_.*ctest\).*/\1/')

      if [[ $recursive_parse_header_result != *"$short_name"* ]]; then
        recursive_parse_header_result="$recursive_parse_header_result $short_name"

        recursive_parse_header_result=$(recursive_parse_header "$dp")
      fi
    fi
  done

  # shellcheck disable=SC2001
  echo "$recursive_parse_header_result" | sed 's/^ //'
}


depends ( )
{
  for cpp in $cpps; do
    obj=$(echo "$cpp" | sed 's/\ /\n/g' | sed 's/.*\/\(.*cpp\)$/\1/' | sed 's/cpp/o/')
    hea="${cpp//cpp/h}"

    unset dep
    unset hea_incl

    dep=$(grep -rnw "$cpp" -e '^#include[ ]*\"src.*\/\(.*h\|test_.*ctest\)' | sed 's/.*\///g' | sed 's/\"//g')

    if [ -e "$hea" ]; then
      unset recursive_parse_header_result
      hea_incl=$(recursive_parse_header "$hea")
    fi

    # shellcheck disable=SC2068
    echo "$obj": ${dep[@]} "$hea_incl"
  done
}


jazz_depends=$(depends)

cd "$jazz_pwd" || return 1


# 1.9 Create an include path list without duplicates

incl_paths=(
    "$mhd_inclpath"
    "$curl_inclpath"
    "$zmq_inclpath"
    "$onnx_rt_inclpath"
    "$onnx_proto_inclpath"
)

# Deduplicate the array
unique_incl_paths=($(printf "%s\n" "${incl_paths[@]}" | awk '!seen[$0]++'))

# Build the neat_includes string
neat_includes=$(printf -- "-I%s " "${unique_incl_paths[@]}")

# Trim any trailing space
neat_includes=$(echo "$neat_includes" | sed 's/[[:space:]]*$//')


# 1.a Dump all variables if debugging

if [[ $mode =~ 'DEBUG' ]]; then
  echo "jazz_pwd            = $jazz_pwd"
  echo "jazz_version        = $jazz_version"
  echo "jz_processor        = $jz_processor"
  echo "jazz_distro1        = $jazz_distro1"
  echo "jazz_distro2        = $jazz_distro2"
  echo "jazz_years          = $jazz_years"
  echo "mhd_inclpath        = $mhd_inclpath"
  echo "mhd_libpath         = $mhd_libpath"
  echo "curl_inclpath       = $curl_inclpath"
  echo "curl_libpath        = $curl_libpath"
  echo "zmq_inclpath        = $zmq_inclpath"
  echo "zmq_libpath         = $zmq_libpath"
  echo "onnx_rt_inclpath    = $onnx_rt_inclpath"
  echo "onnx_rt_libpath     = $onnx_rt_libpath"
  echo "onnx_proto_inclpath = $onnx_proto_inclpath"
  echo "vpath               = $vpath"
  echo "jzpat               = $jzpat"
  echo "cpps                = $cpps"
  echo "objs                = $objs"
  echo "jazz_depends        = $jazz_depends"
  echo "uplifted_bop        = $uplifted_bop"
  echo "uplifted_mod        = $uplifted_mod"
  echo "uplifted_api        = $uplifted_api"
  echo "incl_paths          = $(printf "%s " "${incl_paths[@]}")"
  echo "unique_incl_paths   = $(printf "%s " "${unique_incl_paths[@]}")"
  echo "neat_includes       = $neat_includes"

  printf "\n"
fi


# Section 2. Ask for confirmation. Do nothing if not.
# ----------

if [[ "$1" != "-y" ]]; then
	printf "You will now override:\n\n"
	< _config_/help_on_config.txt grep '  - '
	printf "\n"

	read -p "Do you want to continue? [y/N] " -r
	echo    # (optional) move to a new line

	if [[ $REPLY =~ ^[Yy]$ ]]; then printf "\n"; else exit 1; fi
fi


# Section 3. Start the Kung-fu
# ----------

printf "Writing: server/src/include/jazz_platform.h ... "

echo "$(cat _config_/copyright_notice)

#define JAZZ_VERSION \"$jazz_version\"
#define JAZZ_YEARS \"$jazz_years\"
#define JAZZ_DEBUG_HOME \"$jazz_pwd\"

#define LINUX_DISTRO \"$jazz_distro1\"
#define LINUX_VERSION \"$jazz_distro2\"
#define LINUX_PROCESSOR \"$jz_processor\"

#define LINUX_PLATFORM LINUX_DISTRO \"_\" LINUX_VERSION \"_\" LINUX_PROCESSOR" > server/src/include/jazz_platform.h

printf "Ok.\n"


printf "Writing: server/Makefile ... "

echo "$(cat _config_/makefile_head)

CXXFLAGS     := -std=c++17 -funsigned-char -I. $neat_includes
LINUX        := ${jazz_distro1}_${jazz_distro2}
HOME         := $jazz_pwd
VERSION      := $jazz_version
mhd_libpath  := $mhd_libpath
curl_libpath := $curl_libpath
zmq_libpath  := $zmq_libpath
onnx_libpath := $onnx_rt_libpath

VPATH = $vpath

objects = $objs

# Header file dependencies:

$jazz_depends

$(cat _config_/makefile_tail)" > server/Makefile

printf "Ok.\n"


printf "Writing: server/src/main.dox ... "

echo "/**
\mainpage [Programming documentation for the Jazz Server version \"$jazz_version\"]

$(cat _config_/main_dox_tail)" > server/src/main.dox

printf "Ok.\n"


printf "Writing: docker/base/build_upload.sh ... "

echo "$(cat _config_/base_docker_head)

docker tag jazz_base_stable kaalam/jazz_base:$jazz_version
docker push kaalam/jazz_base:$jazz_version

$(cat _config_/base_docker_tail)" > docker/base/build_upload.sh

chmod 777 docker/base/build_upload.sh

printf "Ok.\n"


printf "Writing: docker/lss/build_upload.sh ... "

echo "$(cat _config_/lss_docker_head)

docker tag jazz_lss_stable kaalam/jazz_lss:$jazz_version
docker push kaalam/jazz_lss:$jazz_version

$(cat _config_/lss_docker_tail)" > docker/lss/build_upload.sh

chmod 777 docker/lss/build_upload.sh

printf "Ok.\n"


printf "Writing: docker/tng/build_upload.sh ... "

echo "$(cat _config_/tng_docker_head)

docker tag jazz_tng_stable kaalam/jazz_tng:$jazz_version
docker push kaalam/jazz_tng:$jazz_version

$(cat _config_/tng_docker_tail)" > docker/tng/build_upload.sh

chmod 777 docker/tng/build_upload.sh

printf "Ok.\n"


printf "Creating folder: server/src/uplifted ... "

mkdir -p server/src/uplifted

printf "Ok.\n"


cd server || return 1

uplifted_incl=""
using_namespace_bop="0"
using_namespace_mod="0"

if [ "$uplifted_bop" != "$uplifted_bop_parent" ]; then
  uplifted_incl+="#include \"$(ls $uplifted_bop_source*.h)\"\n"
else
  if [ "$using_namespace_bop" != "1" ]; then
    uplifted_incl+="using namespace jazz_bebop;\n"
  fi
fi

if [ "$uplifted_mod" != "$uplifted_mod_parent" ]; then
  uplifted_incl+="#include \"$(ls $uplifted_mod_source*.h)\"\n"
else
  if [ "$using_namespace_mod" != "1" ]; then
    uplifted_incl+="using namespace jazz_models;\n"
  fi
fi

if [ "$uplifted_api" != "$uplifted_api_parent" ]; then
  uplifted_incl+="#include \"$(ls $uplifted_api_source*.h)\"\n"
else
  uplifted_incl+="using namespace jazz_main;\n"
fi


cd "$jazz_pwd" || return 1


printf "Writing: server/src/uplifted/uplifted_instances.h ... "

printf "// This file is auto generated, do NOT edit, run ./config.sh instead

$uplifted_incl
extern $uplifted_bop BEBOP;
extern $uplifted_mod MODELS_API;
extern $uplifted_api HTTP_API;\n" > server/src/uplifted/uplifted_instances.h

printf "Ok.\n"


printf "Writing: server/src/uplifted/uplifted_instances.cpp ... "

printf "// This file is auto generated, do NOT edit, run ./config.sh instead

$uplifted_bop BEBOP(&LOGGER, &CONFIG, &CHANNELS, &VOLATILE, &PERSISTED, &CORE);
$uplifted_mod MODELS_API(&LOGGER, &CONFIG, &CHANNELS, &VOLATILE, &PERSISTED, &CORE, &BEBOP);
$uplifted_api HTTP_API(&LOGGER, &CONFIG, &CHANNELS, &VOLATILE, &PERSISTED, &CORE, &BEBOP, &MODELS_API);\n" > server/src/uplifted/uplifted_instances.cpp

printf "Ok.\n"


cat _config_/help_on_done.txt
