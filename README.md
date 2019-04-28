# dmoji
A Linux tool using dmenu to select an emoji and copy it to clipboard. A text file with additional more complex emojis sequences or ASCII arts can be provided via a command line option.
Unlike some other similar tools, dmoji does not use a static text file of emojis : the list is built on the fly straight from the ICU library.

# Requirements

* ICU library and headers
* xsel : to copy data to clipboard
* dmenu : a generic menu for X


# Compile

1) Ensure that you have libicu libraries & headers, as well as xsel & dmenu.

* On a Fedora based machine :

```
$ sudo dnf install libicu libicu-devel dmenu xsel
```

* On a Arch Linux based machine :

```
$ sudo pacman -S icu dmenu xsel
```

2) Compile

```
$ make
```

3) Have a try

```
$ ./dmoji
```

Then choose an emoji and paste it in a text file.

* To install it in /usr/local/bin

```
$ sudo make install
```

# How to use it

## Via the command line

```
$ ./dmoji -h
dmoji version: 0.2, git 089eadf2c95a9316ac027c70662bc4a2a3810537

Calls dmenu with a list of all base emojis available from the ICU library, then copies the selected emoji to clipboard
Options:
 -a <file>      File containing additional data (e.g.: ASCII arts, or more complex Unicode sequences)
                Anything after the separator will be discarded before being sent to the clipboard


Additional data file:
* Each line represents a new entry: the part to be copied, optionally followed by a separator (' ;') and a description to help the search
* Lines starting with a space (i.e.: ' ') are ignored as comments
```


Addiitonal data file:
* Each line represents a new entry: the part to be copied, optionally followed by a separator (' ;') and a description to help the search
* Lines starting with a space (i.e.: ' ') are ignored as comment
```

Generally, you want to add a shortcut in your Window Manager to launch it. For example, in the i3 Window Manager :

```
bindsym $mod+Mod1+e exec --no-startup-id dmoji
```

You will be presented with a list of emoji (character and name) supported by the ICU library. You can then use standard dmenu method to reduce and select a desire emoji and copy it to the clipboard.
