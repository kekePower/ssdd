# ssdd: Simple Shutdown Dialog for Openbox

A simple Shutdown Dialog for Openbox written in C using GTK 

![Project Screenshot](ssdd.png)

![Settings screenshot](ssdd-settings.png)

**Simple Shutdown Dialog (ssdd)** is a simple yet stylish shutdown dialog for Openbox, crafted in C using GTK.

## Why ssdd?

As a long-time Openbox enthusiast, I've always found the default exit dialog a bit lackluster. Modern systems deserve a more refined shutdown experience. While there are other options out there, I figured one more wouldn't hurt, right?

Inspired by the elegant `ssd` from Sawfish, I decided to create my own tailored solution for Openbox.  This way, you can avoid the hassle of installing extra dependencies and enjoy a sleek shutdown dialog that complements your Openbox setup.

## Features

* **Clean and Intuitive Interface:** ssdd presents a clear choice between Shutdown, Reboot, Logout, and Exit options.
* **Clean and minimal code:** `ssdd` is built on a clean and minimal codebase, making it easy to maintain, understand, and extend.
* **Lightweight and Efficient:** ssdd is designed to be fast and resource-friendly, perfectly suited for Openbox's minimalist philosophy.

## Dependencies and Compilation

ssdd requires:

* GTK+ 3.0
* Glib 2 development libraries
* gcc or clang

### Easy Compilation

Edit the `Makefile` or use the following commands:

```shell
% make all     # Compile
% sudo make install # Install to /usr/local
% sudo make install PREFIX=/usr # Install to /usr
```

### Manual compilation

First generate the resources.

```bash
% glib-compile-resources resources.gresource.xml --generate-source --target=resources.c
% glib-compile-resources resources.gresource.xml --generate-header --target=resources.h
```

```bash
# Using GCC:
% gcc ssdd.c resources.c -o ssdd `pkg-config --cflags --libs gtk+-3.0`

# Using Clang:
% clang ssdd.c resources.c -o ssdd `pkg-config --cflags --libs gtk+-3.0`
```

Place the `ssdd` binary in your `$PATH` (e.g., `~/bin`).

### Integrate with Openbox

1. Edit your Openbox menu:

```bash
% sudo nvim /etc/xdg/openbox/menu.xml
```

2. Replace the default Exit entry with:

```xml
<item label="Log Out"><action name="Execute"><execute>ssdd</execute></action></item>
```

3. Reconfigure Openbox:

```bash
% openbox --reconfigure
```

### Contributing

Contributions are welcome! Feel free to open issues or submit pull requests.
