#!/bin/bash
# srb2-legacy AppImage build script
# Rewrite of https://gist.github.com/mazmazz/012f22012c1aea51d7573f0592cfe73a
# Dependencies: sudo apt install make git git-lfs gcc libsdl2-mixer-dev libpng-dev libcurl4-openssl-dev libgme-dev libopenmpt-dev libfuse2 file

for args in "$@"; do
	case "$args" in
		"--noassets")
			noassets=true
			;;
		"--skipbuild")
			skipbuild=true
			;;
		*)
			;;
	esac
done

# Prepare assets with LFS
if [ "$noassets" == true ]; then
	echo "Skipping asset cloning"
else
	git clone https://git.do.srb2.org/STJr/srb2assets-public.git -b SRB2_2.1 assets/appimage
	cd assets/appimage
	git lfs pull
	echo -e "Downloaded assets: \n\n$(git lfs ls-files)"
	cd ../..
	mkdir -p AppDir/usr/bin
	install -m 644 assets/appimage/* AppDir/usr/bin
fi

# Build the application
if [ "$skipbuild" == true ]; then
	echo "Skipping build"
else
	[ "$(uname -m)" == "i686" ] && IS64BIT="" || IS64BIT="64"
	make -C src/ LINUX$IS64BIT=1 -j$(nproc)
fi

# Copy binary to AppDir
install -D bin/lsdl2srb2legacy AppDir/usr/bin/srb2legacy

# Copy icon
cp srb2.png srb2legacy.png

# Create app entrypoint
echo -e \#\!$(dirname $SHELL)/sh >> AppDir/AppRun
echo -e 'HERE="$(dirname "$(readlink -f "${0}")")"' >> AppDir/AppRun
echo -e 'SRB2LEGACYWADDIR=$HERE/usr/bin LD_LIBRARY_PATH=$HERE/usr/lib:$LD_LIBRARY_PATH exec $HERE/usr/bin/srb2legacy "$@"' >> AppDir/AppRun
chmod +x AppDir/AppRun

# Build AppImage
curl --retry 9999 --retry-delay 3 --speed-time 10 --retry-max-time 0 -C - -L https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$(uname -m).AppImage -o linuxdeploy
chmod +x linuxdeploy
NO_STRIP=true ./linuxdeploy --appdir AppDir --output appimage -d srb2legacy.desktop -i srb2legacy.png

# Clean
rm -rf assets/appimage srb2legacy.png linuxdeploy AppDir/
