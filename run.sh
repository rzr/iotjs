#!/bin/bash
# -*- coding: utf-8 -*-
#{
# Copyright 2017 Samsung Electronics France SAS
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#}

#set -x

project="iotjs"

#{TODO: Update here if needed
user="rzr"
org="TizenTeam"
branch="sandbox/${user}/master"
#}

url="https://github.com/${org}/${project}.git#${branch}"
run_url="https://raw.githubusercontent.com/${org}/${project}/${branch}/run.sh"
tag="${project}/${branch}"
version="latest"
outdir="${PWD}/tmp/out"

[ -d '.git' ] && which git && branch=$(git rev-parse --abbrev-ref HEAD) ||:


usage_()
{
    cat<<EOF
Usage:
$0
or
curl -sL "${run_url}" | bash -

EOF
}


die_()
{
    errno=$?
    echo "error: [$errno] $@"
    exit $errno
}


setup_debian_()
{
    which docker || sudo apt-get install docker.io

    sudo apt-get install qemu qemu-user-static binfmt-support
    sudo update-binfmts --enable qemu-arm
}


setup_()
{
    docker version && return $? ||:

    if [ -r /etc/debian_version ] ; then
        setup_debian_
    else
        cat<<EOF
warning: OS not supported
Please ask for support at:
${url}
EOF
        cat /etc/os-release ||:
    fi

    docker version && return $? ||:
    docker --version || die_ "please install docker"
    groups | grep docker \
        || sudo addgroup ${USER} docker \
        || die_ "${USER} must be in docker group"
    su -l $USER -c docker version \
        && { su -l $USER -c "$@" ; exit $?; } \
        || die_ "unexpected error"
}


prep_()
{
    echo "Prepare: "
    cat /etc/os-release
    docker version || setup_
}


build_()
{
    basename=$(basename -- "$0")
    container="${project}/${user}/${branch}"
    container=$(echo "${container}" | sed -e 's|/|-|g')
    dir="/usr/local/src/${project}/"

    if [ "run.sh" = "${basename}" ] ; then
        docker build -t "$tag" .
    else
        docker build -t "$tag" "$url"
    fi
    docker rm "$container" ||:
    docker create --name "$container" "$tag"
    rm -rf "${outdir}"
    mkdir -p "${outdir}"
    docker cp "$container:$dir" "${outdir}"
    ls "${outdir}/"
    find "${outdir}" -iname "*.deb"
}


test_()
{
    curl -sL "${run_url}" | bash -
}


main_()
{
    usage_ "$@"
    prep_ "$@"
    build_ "$@"
}


main_ "$@"
