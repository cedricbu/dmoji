# dmoji
A Linux tool using dmenu to select an emoji and copy it to clipboard. A text file with additional more complex emojis sequences or ASCII arts can be provided via a command line option.
Unlike some other similar tools, dmoji does not use a static text file of emojis : the list is built on the fly straight from the ICU library.

# Requirements

* ICU library and headers
* `xsel` : to copy data to clipboard
* A dynamic menu for X (`dmenu` and `rofi` are currently supported)


# Compile

1) Ensure that you have libicu libraries & headers, as well as xsel & dmenu.

* On a Fedora based machine :

```sh
$ sudo dnf install libicu libicu-devel dmenu xsel
```

* On a Arch Linux based machine :

```sh
$ sudo pacman -S icu dmenu xsel
```

2) Compile

```sh
$ make
```

3) Have a try

3.a) with dmenu

```sh
$ ./dmoji
```

3.b) with rofi

```sh
$ ./dmoji -r
```

Then choose an emoji and paste it in a text file.

4) You can also add a single file, or a directory containing files, that will be appended at the end of the list

For example, use one of these commands to include all the files provided in the `additional-entries` directory, or only one file.

```sh
$ ./dmoji -a ./additional-entries/
  # -or- 
$ ./dmoji -a ./additional-entries/ascii-arts.txt
```

Each lines (except those starting with a `<space>`, used for comments) will be appended to the list of emojis. If such a line is selected, **only** the part preceding the separator ` ;` (character <space> followed by character semi-colon) will be used.

Since, in the example above, the additional entries contain 'ASCII' as part of their description, type 'ascii' in dmenu/rofi to filter them.

* To install it in /usr/local/bin

```sh
$ sudo make install
```

# How to use it

## Via the command line

```sh
$ ./dmoji -h
dmoji version: 0.5-2-g186a4b0, git 186a4b05b6cce167cb4498335e1f47dd0dd3cf68

Calls dmenu or rofi with a list of all base emojis available from the ICU library, then copies the selected emoji to clipboard
Options:
 -r     Use Rofi instead of dmenu (Rofi will be started in dmenu mode)
 -a <path>  File/directory containing additional data (e.g.: ASCII arts, or more complex Unicode sequences)
        Anything after the separator will be discarded before being sent to the clipboard


Additional data file:
* Each line represents a new entry: the part to be copied, optionally followed by a separator (' ;') and a description to help the search
* Lines starting with a space (i.e.: ' ') are ignored as comments


Return value:
0: Success, data sent to clipboard
1: No data (user exited without selection)
>1: Something didn't go as planned
```

## Via a Window Manager shortcut

Generally, you want to add a shortcut in your Window Manager to launch it. For example, in the i3 Window Manager :

```
bindsym $mod+Mod1+e exec --no-startup-id dmoji
```

You will be presented with a list of emoji (character and name) supported by the ICU library. You can then use standard dmenu method to reduce and select a desire emoji and copy it to the clipboard.
