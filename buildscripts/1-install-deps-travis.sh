#!/bin/bash
set -x
set -e
set -u

sudo mkdir -p /data/bin
sudo chmod uga+rwx /data/bin

GITHUB_RELEASE_TOOL_USER="paulthomson"
GITHUB_RELEASE_TOOL_VERSION="v1.0.9.1"


mkdir deps
cd deps

if [ "$(uname)" == "Darwin" ];
then
  GITHUB_RELEASE_TOOL_ARCH="darwin_amd64"
  wget https://github.com/paulthomson/build-angle/releases/download/v-592879ad24e66c7c68c3a06d4e2227630520da36/Darwin-x64-Release.zip
  unzip Darwin-x64-Release.zip
fi

if [ "$(uname)" == "Linux" ];
then
  GITHUB_RELEASE_TOOL_ARCH="linux_amd64"
  wget https://github.com/paulthomson/build-angle/releases/download/v-592879ad24e66c7c68c3a06d4e2227630520da36/Linux-x64-Release.zip
  unzip Linux-x64-Release.zip
fi

cd ..


pushd /data/bin
wget "https://github.com/${GITHUB_RELEASE_TOOL_USER}/github-release/releases/download/${GITHUB_RELEASE_TOOL_VERSION}/github-release_${GITHUB_RELEASE_TOOL_VERSION}_${GITHUB_RELEASE_TOOL_ARCH}.tar.gz"
tar xf "github-release_${GITHUB_RELEASE_TOOL_VERSION}_${GITHUB_RELEASE_TOOL_ARCH}.tar.gz"
popd


