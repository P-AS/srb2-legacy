#!/bin/bash
# srb2-legacy AppImage build script
# Dependencies: sudo apt install make git git-lfs gcc libsdl2-mixer-dev libpng-dev libcurl4-openssl-dev libgme-dev libopenmpt-dev libfuse2 file

# Prepare assets with LFS
git clone https://git.do.srb2.org/STJr/srb2assets-public.git -b SRB2_2.1 assets/appimage
cd assets/appimage
git lfs pull
echo -e "Downloaded assets: \n\n$(git lfs ls-files)"
cd ../..

# Clone the repo and build the application
[ "$(uname -m)" == "i686" ] && IS64BIT="" || IS64BIT="64"
make -C src/ LINUX$IS64BIT=1 -j$(nproc)

# Copy files to bin
install -D bin/lsdl2srb2legacy AppDir/usr/bin/srb2legacy
install assets/appimage/* AppDir/usr/bin

# Copy icon
cp srb2.png srb2legacy.png

# create app entrypoint
echo -e \#\!$(dirname $SHELL)/sh >> AppDir/AppRun
echo -e 'HERE="$(dirname "$(readlink -f "${0}")")"' >> AppDir/AppRun
echo -e 'SRB2LEGACYWADDIR=$HERE/usr/bin LD_LIBRARY_PATH=$HERE/usr/lib:$LD_LIBRARY_PATH exec $HERE/usr/bin/srb2legacy "$@"' >> AppDir/AppRun
chmod +x AppDir/AppRun

# Build AppImage
curl --retry 9999 --retry-delay 3 --speed-time 10 --retry-max-time 0 -C - -L https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$(uname -m).AppImage -o linuxdeploy
chmod +x linuxdeploy
NO_STRIP=true ./linuxdeploy --appdir AppDir --output appimage -d srb2legacy.desktop -i srb2legacy.png

# clean
rm -rf assets/appimage srb2legacy.png linuxdeploy AppDir/
