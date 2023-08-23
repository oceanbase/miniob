#!/bin/bash

# generate ssh private key by env argument

echo "Try to clone repository..."

SSH_DIR=/root/.ssh

if [ "$PRIVATE_KEY" = "" ]; then
    echo "ENV PRIVATE_KEY is not set."
else

    if [ -d "${SSH_DIR}" ]; then
        echo "folder ~/.ssh exists..."
    else

        echo "Generating SSH private rsa key..."

        mkdir -p ${SSH_DIR}

        echo "-----BEGIN OPENSSH PRIVATE KEY-----" >${SSH_DIR}/id_rsa
        echo $PRIVATE_KEY | tr -s " " "\n" | sed "1,4d" | head -n -4 | xargs -L 1 >>${SSH_DIR}/id_rsa
        echo "-----END OPENSSH PRIVATE KEY-----" >>${SSH_DIR}/id_rsa

        chmod 700 ${SSH_DIR}
        chmod 600 ${SSH_DIR}/id_rsa

        # add SSH key fingerprint to known_hosts
        echo $REPO_ADDR | awk -F'[@:]' '{print $2}' | xargs ssh-keyscan $1 >>${SSH_DIR}/known_hosts

        echo "SSH private rsa key generated!"
    fi

fi

# check if source code exists

REPO_DIR=/root/source/miniob

if [ -d "${REPO_DIR}" ]; then
    cd ${REPO_DIR}
    is_git_repo=$(git rev-parse --is-inside-work-tree 2>&1)

    if [ "${is_git_repo}" = "true" ]; then
        echo "The source code has been cloned!"
        exit 0
    else
        echo "WARNING: /root/source/miniob exists but it is not a git repository."
        exit 0
    fi
fi

# clone code.
if [ "$REPO_ADDR" = "" ]; then
    echo "ENV REPO_ADDR is not set."
    exit 0
else
    WORK_DIR=/root/source
    mkdir -p ${WORK_DIR}

    git clone $REPO_ADDR ${REPO_DIR}
fi

