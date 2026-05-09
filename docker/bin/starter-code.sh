#!/bin/bash

# generate ssh private key by env argument

echo "Try to clone repository..."

SSH_DIR=/root/.ssh
REPO_DIR=/root/source/miniob

clone_repo() {
    local repo_addr="$1"

    git clone "$repo_addr" "${REPO_DIR}"
}

extract_repo_host() {
    local repo_addr="$1"

    if [[ "$repo_addr" =~ ^git@([^:]+): ]]; then
        echo "${BASH_REMATCH[1]}"
    elif [[ "$repo_addr" =~ ^https?://([^/]+)/ ]]; then
        echo "${BASH_REMATCH[1]}"
    fi
}

https_to_ssh_repo() {
    local repo_addr="$1"

    if [[ "$repo_addr" =~ ^https://github\.com/([^/]+/[^/]+)(\.git)?$ ]]; then
        echo "git@github.com:${BASH_REMATCH[1]}.git"
    elif [[ "$repo_addr" =~ ^https://github\.com/([^/]+/[^/]+)$ ]]; then
        echo "git@github.com:${BASH_REMATCH[1]}.git"
    else
        echo ""
    fi
}

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
        repo_host=$(extract_repo_host "$REPO_ADDR")
        if [ "$repo_host" != "" ]; then
            ssh-keyscan "$repo_host" >>${SSH_DIR}/known_hosts
        fi

        echo "SSH private rsa key generated!"
    fi

fi

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

    if ! clone_repo "$REPO_ADDR"; then
        ssh_repo_addr=$(https_to_ssh_repo "$REPO_ADDR")
        if [ "$ssh_repo_addr" != "" ]; then
            echo "HTTPS clone failed, retrying with SSH: ${ssh_repo_addr}"
            if ! clone_repo "$ssh_repo_addr"; then
                exit 1
            fi
        else
            exit 1
        fi
    fi
fi

