# Maintainer: Roboguy420 <wahaller@proton.me>
pkgname='gex2ps1modelexporter'
pkgver=5.r18.g4907af0
pkgrel=1
epoch=
pkgdesc="Command line program for exporting Gex 2 PS1 models"
arch=('x86_64')
url="https://github.com/Roboguy420/Gex2PS1ModelExporter"
license=('GPL')
depends=('libpng' 'tinyxml2' 'glibc')
makedepends=('git' 'cmake')
source=('gex2ps1modelexporter::git+https://github.com/Roboguy420/Gex2PS1ModelExporter.git')
md5sums=('SKIP')

pkgver() {
	cd "$srcdir/$pkgname"
	git describe --tags | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g'
}

build() {
	cd "$srcdir/$pkgname"
	mkdir "out"
	cd "out"
	cmake .. -G "Unix Makefiles" --preset x64-release -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DUSE_SHARED_LIBRARIES=1
	cd "build/x64-release"
	cmake --build .
}

package() {
	cd "$srcdir/$pkgname/out/build/x64-release"
	sudo cmake --install .
}