pkgname=holonight-greeter
pkgver=0.1.0
pkgrel=1
pkgdesc='HoloNight greetd greeter'
arch=(x86_64)
url='https://github.com/lebedenko/holonight-greeter'
license=(GPL-3.0-or-later)
depends=(qt6-base qt6-declarative holonight-qt greetd cage accountsservice)
makedepends=(cmake ninja tomlplusplus gtest)
backup=('etc/holonight/greeter.toml')
source=()
sha256sums=()
build() {
  cmake -S "$startdir" -B "$srcdir/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_TESTING=OFF
  cmake --build "$srcdir/build"
}

package() {
  DESTDIR="$pkgdir" cmake --install "$srcdir/build"
}
