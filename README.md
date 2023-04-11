# dmoji
A Linux tool using dmenu to select an emoji and either copy it to clipboard, or type it directly in the active window. One of more text files with additional more complex emojis sequences or ASCII arts can be provided via a command line option.
Unlike some other similar tools, dmoji does not use a static text file of emojis : the list is built on the fly straight from the ICU library.

# Requirements

* ICU library and headers
* A dynamic menu for X (`dmenu` and `rofi` are currently supported)
* `xsel`: to copy data to clipboard
* `xdotool`: to directly type the selected entry in the active window


# Compile

1) Ensure that you have libicu libraries & headers, as well as:
- xsel and/or xdotool
- dmenu and/or rofi

* On a Fedora based machine :

```sh
$ sudo dnf install libicu libicu-devel dmenu xsel xdotool
```

* On a Arch Linux based machine :

```sh
$ sudo pacman -S icu dmenu xsel xdotool
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

3.c) You can also add a single file, or a directory containing files, that will be appended at the end of the list

For example, use one of these commands to include all the files provided in the `additional-entries` directory, or only one file.

```sh
$ ./dmoji -a ./additional-entries/
  # -or- 
$ ./dmoji -a ./additional-entries/ascii-arts.txt
```

Each lines (except those starting with a `<space>`, used for comments) will be appended to the list of emojis. If such a line is selected, **only** the part preceding the separator ` ;` (character <space> followed by character semi-colon) will be used.

Since, in the example above, the additional entries contain 'ASCII' as part of their description, type 'ascii' in dmenu/rofi to filter them.

4) To install it in /usr/local/bin

```sh
$ sudo make install
```

5) Once installed, and usable without a terminal, you can use the `-t` option to type the result directly in the active window, instead of having to paste it.

# How to use it

## Via the command line

```sh
$ ./dmoji -h
dmoji version: 0.5-5-gdaf9798, git daf9798396cde003fe9b3b1a456d0eb4633432d2

Calls dmenu or rofi with a list of all base emojis available from the ICU library, then copies the selected emoji to clipboard
Options:
 -t		Feed the active window with the selection, instead of copying it to the clipboard (requires xdotool)
 -r		Use Rofi instead of dmenu (Rofi will be started in dmenu mode)
 -a <path>	File/directory containing additional data (e.g.: ASCII arts, or more complex Unicode sequences)
		Anything after the separator will be discarded before being sent to the clipboard


Additional data file:
* Each line represents a new entry: the part to be copied, optionally followed by a separator (' ;') and a description to help the search
* Lines starting with a space (i.e.: ' ') are ignored as comments


Return value:
0: Success, data sent to clipboard
1: No data (user exited without selection)
>1: Something didn't go as planned```

## Via a Window Manager shortcut

Generally, you want to add a shortcut in your Window Manager to launch it. For example, in the i3 Window Manager :

```
bindsym $mod+Mod1+e exec --no-startup-id dmoji -tra /path/to/additional/entries/
```

You will be presented with a list of emoji (character and name) supported by the ICU library. You can then use standard dmenu method to reduce and select a desire emoji and copy it to the clipboard.
