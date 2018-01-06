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

set -x

project="iotjs"
arch="armhf"
user="rzr"
org="TizenTeam"
branch="sandbox/${user}/devel/master"
url="https://github.com/${org}/${project}.git#${branch}"
run_url="https://raw.githubusercontent.com/${org}/${project}/${branch}/run.sh"
tag="${project}/$arch"
version="latest"
deb_version="0.0-0"
outdir="${PWD}/tmp/out"


usage_()
{
    cat<<EOF
Usage:
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
    which docker || die_ "please install docker"

    groups | grep docker \
        || sudo addgroup ${USER} docker \
        && echo "TODO: type 'su -l $USER' and run script again, else it may fail"
    docker version || su -l $USER -c docker version
    docker version || su -l $USER -c "$@"
    docker version || die_ "please configure docker"
}


check_()
{
    cat /etc/os-release
    docker --version || setup_
    docker --version
}


build_()
{
    container="${project}-${arch}-${version}"
    file="/usr/local/src/${project}_${deb_version}_${arch}.deb"
    deb="${project}_${deb_version}_${arch}.deb"
    file="/usr/local/src/$deb"

    check_
    docker build -t "$tag" "$url"
    docker rm "$container"
    docker create --name "$container" "$tag"
    mkdir -p "${outdir}"
    docker cp "$container:$file" "${outdir}"
    ls -l "${outdir}"
}


test_()
{
    curl -sL "${run_url}" | bash -
}


main_()
{
    usage_ "$@"
    build_ "$@"
}


main_ "$@"
