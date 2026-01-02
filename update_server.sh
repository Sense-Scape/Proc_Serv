#!/bin/bash

cd build_arm

# Default value for REMOTE_HOST
REMOTE_HOST=""
REMOTE_USER=""
REMOTE_PASS=""
REMOTE_PATH="/opt/spectra-procserv/"
NEW_FILE_PATH="./ProcServ"
REMOTE_TMP_PATH="./ProcServ"
REMOTE_SERVICE_NAME="spectra-procserv.service"

# Function to display usage
usage() {
    echo "Usage: $0 -h <remote_host> [-u <user>] [-p <password>]"
    exit 1
}

# Parse options
while getopts ":h:u:p:" opt; do
    case ${opt} in
        h )
            REMOTE_HOST="$OPTARG"
            ;;
        u )
            REMOTE_USER="$OPTARG"
            ;;
        p )
            REMOTE_PASS="$OPTARG"
            ;;
        * )
            usage
            ;;
    esac
done

# Check if REMOTE_HOST is set
if [ -z "$REMOTE_HOST" ]; then
    echo "Error: Remote host not specified."
    usage
fi

# Check if REMOTE_HOST is set
if [ -z "$REMOTE_PASS" ]; then
    echo "Error: password not specified."
    usage
fi

# Step 1: Copy the file to the remote PC
echo "Copying the file to the remote machine..."
if [ -n "$REMOTE_PASS" ]; then
    if ! command -v sshpass >/dev/null 2>&1; then
        echo "Error: sshpass is required for password authentication. Install it or omit -p."
        exit 1
    fi
    sshpass -p "$REMOTE_PASS" scp -o StrictHostKeyChecking=no ${NEW_FILE_PATH} ${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_TMP_PATH}
else
    scp ${NEW_FILE_PATH} ${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_TMP_PATH}
fi
echo "Files Copied"

sshpass -p "$REMOTE_PASS" ssh -o StrictHostKeyChecking=no ${REMOTE_USER}@${REMOTE_HOST} << EOF
    echo "Changing permissions for the new file..."
    echo "$REMOTE_PASS" | sudo -S chmod +wrx ${NEW_FILE_PATH}

    echo "Moving the new file to the target location..."
    echo "$REMOTE_PASS" | sudo -S mv ${NEW_FILE_PATH} ${REMOTE_PATH}

    echo "Restarting service"
    echo "$REMOTE_PASS" | sudo -S systemctl restart ${REMOTE_SERVICE_NAME}
EOF

