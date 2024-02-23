VERSION="8.2.0"
CPU_TARGET="x86_64"

cd qemu-$VERSION || exit 1

CFLAGS="-O3" ./configure --disable-werror \
  --enable-linux-user --enable-system --disable-gtk --disable-sdl --disable-vnc \
  --target-list="${CPU_TARGET}-linux-user ${CPU_TARGET}-softmmu" --enable-pie || exit 1

echo "[+] Configuration complete."

echo "[*] Attempting to build QEMU (fingers crossed!)..."

make -j6 || exit 1

echo "[+] Build process successful!"

echo "[*] Copying binary..."

cp -f "build/${CPU_TARGET}-linux-user/qemu-${CPU_TARGET}" "../../afl-qemu-trace" || exit 1
cp -f "build/${CPU_TARGET}-softmmu/qemu-system-${CPU_TARGET}" "../../afl-qemu-system-trace" || exit 1

cd ..
ls -l ../afl-qemu-trace || exit 1
ls -l ../afl-qemu-system-trace || exit 1

echo "[+] Successfully created '../afl-qemu-trace and ../afl-qemu-system-trace'."