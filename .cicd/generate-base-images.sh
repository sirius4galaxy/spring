#!/usr/bin/env bash
set -eo pipefail

[[ -z $1 ]] && echo "Must provide the distro IMAGE_TAG name (example: ubuntu-18.04)" && exit 1
export IMAGE_TAG=$1

function determine-hash() {
    # Determine the sha1 hash of all dockerfiles in the .cicd directory.
    [[ -z $1 ]] && echo "Please provide the files to be hashed (wildcards supported)" && exit 1
    echo "Obtaining Hash of files from $1..."
    # Collect all files, hash them, then hash those.
    HASHES=()
    for FILE in $(find $1 -type f); do
        HASH=$(sha1sum $FILE | sha1sum | awk '{ print $1 }')
        HASHES=($HASH "${HASHES[*]}")
        echo "$FILE - $HASH"
    done
    export DETERMINED_HASH=$(echo ${HASHES[*]} | sha1sum | awk '{ print $1 }')
    export HASHED_IMAGE_TAG="${IMAGE_TAG}-${DETERMINED_HASH}"
}

function generate_docker_image() {
    # If we cannot pull the image, we build and push it first.
    docker login -u $DOCKERHUB_USERNAME -p $DOCKERHUB_PASSWORD
    cd ./.cicd
    docker build -t eosio/producer:ci-${HASHED_IMAGE_TAG} -f ./${IMAGE_TAG}.dockerfile .
    docker push eosio/producer:ci-${HASHED_IMAGE_TAG}
    cd -
}

function docker_tag_exists() {
    ORG_REPO=$(echo $1 | cut -d: -f1)
    TAG=$(echo $1 | cut -d: -f2)
    EXISTS=$(curl -s -H "Authorization: Bearer $(curl -sSL "https://auth.docker.io/token?service=registry.docker.io&scope=repository:${ORG_REPO}:pull" | jq --raw-output .token)" "https://registry.hub.docker.com/v2/${ORG_REPO}/manifests/$TAG")
    ( [[ $EXISTS =~ '404 page not found' ]] || [[ $EXISTS =~ 'manifest unknown' ]] ) && return 1 || return 0
}

determine-hash ".cicd/${IMAGE_TAG}.dockerfile"
[[ -z $DETERMINED_HASH ]] && echo "DETERMINED_HASH empty! (check determine-hash function)" && exit 1
echo "Looking for $IMAGE_TAG-$DETERMINED_HASH"
if docker_tag_exists eosio/producer:ci-${HASHED_IMAGE_TAG}; then
    echo "eosio/producer:ci-${HASHED_IMAGE_TAG} already exists"
else
    generate_docker_image
fi
