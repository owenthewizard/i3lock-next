# i3lock-next

i3lock-next is a bash script and C helper program much like [i3lock-fancy](https://github.com/meskarune/i3lock-fancy). i3lock-next aims to be much faster by using [Imlib2](https://docs.enlightenment.org/api/imlib2/html/index.html) rather than ImageMagick, and being written (mostly) in C.

#### Now with support for multiple monitors!

![screenshot](screenshot.png)

[Video](video.mp4)

## Dependencies

- [i3lock-color](https://github.com/chrjguill/i3lock-color) - a fork of i3lock that supports custom ring colors
- Imlib2
- bash
- fontconfig
- libXrandr

## Installation

```
make
make install
```
For custom prefix:
```
make install PREFIX=/your/custom/prefix
```

## Usage

```
i3lock-next [-h|--help] [font] [size]

Options:
    -h, --help  Display this help text.

    font        Font to to use, default sans.

    size        Size of font, default 18.
```
Example:
```
i3lock-next "Open Sans" 18
i3lock-next # Defaults to Sans/18
```
