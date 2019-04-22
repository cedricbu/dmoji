# dmoji
A Linux tool using dmenu to select an emoji and copy it to clipboard

# Requirements

* ICU library and headers
* xsel : to copy data to clipboard
* dmenu : a generic menu for X


# Compile

```
$ make
$ sudo make install
```

# How to use it

Generally, you want to add a shortcut in your Window Manager to launch it. For example, in i3 WM :

```
bindsym $mod+Mod1+e exec --no-startup-id dmoji
```

You will be presented with a list of emoji (character and name) supported by the ICU library. You can then use standard dmenu method to reduce and select a desire emoji and copy it to the clipboard.
