# Swaykbdd: Automatic keyboard layout switching in Sway

![CI](https://github.com/artemsen/swaykbdd/workflows/CI/badge.svg)

The _swaykbdd_ utility can be used to automatically change the keyboard layout
on a per-window or per-tab basis.

## Usage

Just run `swaykbdd`. For automatic start, add to the Sway config file the
following command:
`exec swaykbdd`

## Build and install

[![Packaging status](https://repology.org/badge/tiny-repos/swaykbdd.svg)](https://repology.org/project/swaykbdd/versions)

```
meson build
ninja -C build
sudo ninja -C build install
```

Arch users can install the program via [AUR](https://aur.archlinux.org/packages/swaykbdd).
