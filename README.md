# ssdd

A simple Shutdown Dialog for Openbox written in C using GTK

## Why?

I just bought a new laptop and on my workstation I was using ssd from Sawfish which I loved. I didn't want to go through all the steps of installing the necessary libraries and dependencies to get it to work, so I decided to create my own.

## Dependencies and compilation

This app requires GTK+ 3.0 development libraries and gcc.

I am using this command to compile the program:
```shell
% gcc ssdd.c -o ssdd `pkg-config --cflags --libs gtk+-3.0`
```

This produces the binary `ssdd` which you can place in your $PATH.

## Configure Openbox to use it.

`% sudo nvim /etc/xdg/openbox/menu.xml`

Find the line with the standard Openbox Exit option and change it to
`    <item label="Log Out"><action name="Execute"><execute>/home/stig/bin/ssdd</execute></item>`

The reconfigure Openbox to use the new setting.
`% openbox --reconfigure`
