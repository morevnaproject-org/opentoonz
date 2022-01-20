SCRIPT_FILE=`realpath "$0"`
SCRIPT_DIR=`dirname "$SCRIPT_FILE"`

docker run -it \
    --name "dev-ot" --rm \
    -v $SCRIPT_DIR/../../../opentoonz:/root/opentoonz \
    -v $SCRIPT_DIR/../../../opentoonz/toonz/build.docker:/root/opentoonz/toonz/build \
    --cap-add SYS_ADMIN --cap-add MKNOD --device=/dev/fuse \
    --security-opt apparmor:unconfined \
    morevna/dev-ot
