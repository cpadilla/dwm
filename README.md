# dwm
My patched version of dwm based on the git.suckless.org/dwm repo 

## Instructions to build on Arch Linux
Run the following command

`makepkg -Acs --skipchecksums -f`

This should build the package. Then install it using pacman. For example:

`sudo pacman -U dwm-6.1-2-x86_64.pkg.tar.xz`

By default, Shift+Alt+q will shut down dwm, allowing you to restart after you've installed your changes.

## Instructions to build on FreeBSD

The freebsd branch contains the files needed to add to the port (in the `/usr/ports/x11-wm/dwm/files` directory).

Once those files are added, you can make and install the system using the `config.h` file with the following command:

```
sudo make DWM_CONF=/usr/ports/x11-wm/dwm/files/config.h install
```
