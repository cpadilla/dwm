pkgname=dwm
pkgver=6.4
pkgrel=2
pkgdesc="A dynamic window manager for X"
url="http://dwm.suckless.org"
arch=('i686' 'x86_64')
license=('MIT')
options=(zipman)
depends=('libx11' 'libxinerama' 'libxft' 'freetype2' 'libxrandr')
install=dwm.install
source=(
    http://dl.suckless.org/$pkgname/$pkgname-$pkgver.tar.gz
    # $pkgname-$pkgver::git+git://git.suckless.org/dwm#tag=6.2
    config.h
)
_patches=(
    01-uselessgap.diff
    02-systray.diff
    03-bottomstack.diff
    04-deck.diff
    05-attachbottom.diff
    06-pertag.diff
    07-statusallmons.diff
    08-rainbowtags.diff
    09-xrandrmonitors.diff
    10-fullscreenpermonitor.diff
    11-togglevmon.diff
)

source=(${source[@]} ${_patches[@]})

build() {
  cd $srcdir/$pkgname-$pkgver

  for p in "${_patches[@]}"; do
    echo "=> $p"
    # normalize line endings just in case
    sed -i 's/\r$//' "../$p"
  
    if patch --dry-run -Np1 -i "../$p" >/dev/null 2>&1; then
      patch -Np1 --follow-symlinks -i "../$p" || return 1
    elif patch --dry-run -Np0 -i "../$p" >/dev/null 2>&1; then
      patch -Np0 --follow-symlinks -i "../$p" || return 1
    else
      echo "Failed to apply $p with -p1 or -p0"
      return 1
    fi
  done

  cp $srcdir/config.h config.h

  make X11INC=/usr/include/X11 \
       X11LIB=/usr/lib/X11 \
       FREETYPEINC=/usr/include/freetype2 \
       XINERAMAFLAGS='-DXINERAMA' \
       XINERAMALIBS='-lXinerama' \
       VERSION="${pkgver}"
}

package() {
  cd $srcdir/$pkgname-$pkgver
  make PREFIX=/usr DESTDIR=$pkgdir install
  install -m644 -D LICENSE $pkgdir/usr/share/licenses/$pkgname/LICENSE
  install -m644 -D README $pkgdir/usr/share/doc/$pkgname/README
}

md5sums=('8bb00d4142259beb11e13473b81c0857'
         '939f403a71b6e85261d09fc3412269ee'
         'db8a40d51592ca6e2c7bc1b99d960e10'
         '362e07f0f042875b84d7739d9d8855c4'
         '4ba509b3b93f7b1418dc703c70de536f'
         '689534c579b1782440ddcaf71537d8fd'
         '57b1a8f21b61c55f906d7cc075111613'
         'e3faeea09a554bbbce29c4d480b0ca41'
         '1f0244803c0188f1b6f4e5794e7f5ca2'
         'ed11483bfccbf65ff9714c0ca4e7bb23'
         'bc6240f3adadf604a450f6375badec61'
         'a92ee04c33b1082da61b55d3617249eb'
         '29213fff0d93fc3a7183948f6c792ac5')
