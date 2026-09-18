# Welcome to WL_Auto-Clicker.Appimagelicker!

> This auto-clicker was specifically designed for Wayland architecture in Ubuntu 26.3 . No testing has been performed for X11.
> Please refer to another auto-clicker such as [XClicker](https://github.com/robiot/xclicker).

WL_Auto-Clicker.Appimagelicker was born out of my requirement and complete inability to locate an auto-clicker compatible with Wayland.
(Most of the ones I found either had no customisable hotkey option, or automatically mapped to F8, which was an issue).\
I considered this a good opportunity to develop as project of my own, as well as return something to the open source community that I have received so much from.\
Feel free to reach me at my email `pranjalsharma3554@gmail.com` for any issues/advices/suggestions related to WL_Auto-Clicker.Appimagelicker.
There are some features that I would like to implement in the future, which will be listed below. I'll try my best to accommodate feedback in there too. \
This is my first project, and I'll keep developing more in the future! \
\
List of topics:
- [Features](#features)
    - [CPS](#cps--clicks-per-second)
    - [Supported options](#supported-options)
    - [Hotkey](#hotkey)
    - [Commands](#commands)
-  [Setup](#setup)
- [Contributions](#contributions)

## Features
* ### CPS / Clicks per second
WL_Auto-Clicker.Appimagelicker was tested upto `1,000,000` cps on the custom made testbench provided in the repo. It was also tested on multiple CPS Test websites. The most preferable among those would be [Drag Click Test](https://www.clickmetrik.com/drag-click-test). The following table shows
the results. At each CPS, 3 runs were performed and their average is reported:
| Testing CPS | Observed CPS (Average of 3 runs) | Deviation % | 
|-------------|----------------------------------|-------------|
| 7 | 7.00 | +0.000 |
| 20 | 20.00 | +0.000 |
| 100 | 100.03 | +0.030 |
| 250 | 250.08 | +0.032 |
| 600 | 599.91 | -0.015 |
| 2000 | 2000.51 | +0.026 |

Meaningful tests beyond this were not practical, the browsers, as expected, would either hang or crash.\
Though I personally believe that these results should be impressive enough.

- The CPS also drops down to upto 0.01 cps, which is about 100 seconds per click. This is useful for some delay-oriented actions.

>Note: Using cps values higher than 2000 is discouraged. In the usual case, your program may freeze. In worst, your system may crash.

* ## Supported Options
Presently, the mouse only supports lmb (Left Mouse Button) and rmb (Right Mouse Button). The ability to position mouse at a certain position is one of future possibility I would like to add.

* ## Hotkey
WL_Auto-Clicker.Appimagelicker runs in the background in a shell environment, a terminal usually. The clicking function is toggled (start or stop) distinctly from the main program using the hotkey.\
>Note: Since the auto-clicker is run in the terminal shell, any hotkey combinations that affect the terminal functionality must be kept in mind, and avoided as the auto-clicker's hotkey.
>Alternatively, just pressing Ctrl+C both ends the auto-clicker program and closes the terminal window.

The hotkey must be manually written down in the terminal window. This was implemented as so to prevent clashes with terminal's own key listening.
Refer to [Commands](#commands) for more details.\
Hotkey will be written in the form of "`key1`+`key2`+`key3`", where each of key1, key2 or key3 can be one of those listed in the table [below](#permitted-keys-for-hotkey-combination).
Clearly, a maximum of 3 keys in an input combination is permitted.\While keeping the hotkey as a single or a pair of keys is permitted, the user is advised to handle any hotkey collisions with other programs.\
For the following keys:
```text
` ' \
```
Enclose these in single quotes `'` in the terminal shell to pass them successfully. For `'` itself, use double quotes `"`. Example usage:
- `"ctrl+'"`
- `'ctrl+\'`
- ``"ctrl+`"``

### Permitted keys for Hotkey Combination
| Type of key | Permitted Keys |
|-------------|----------------|
| Alphanumeric|  `0-9` , `a-z` |
|Function|`F1-F12`|
|Special characters| `` ` ``, `'`, `-`, `=`, `[`,`]`,`\`,`;`,`,`,`.`,`/`|
|Modifier keys|`ctrl`,`shift`,`alt`

## Commands
The name of the executable will be WL_Auto-Clicker.Appimage.

* ### Helper command
```
./WL_Auto-Clicker.Appimage --help
```
Displays auto-clicker usage commands. These command are listed below.
* ### Hotkey Setting command
```
./WL_Auto-Clicker.Appimage --hotkey <hotkey-string>
```
Sets up hotkey.\
The program displays the entered hotkey combination. if the entered combination has no issues, it will successfully set the hotkey to the combination. Otherwise, the issue is displayed.\
Example Usage:
```
./WL_Auto-Clicker.Appimage --hotkey 'ctrl+,'
./WL_Auto-Clicker.Appimage --hotkey '`'
./WL_Auto-Clicker.Appimage --hotkey 'ctrl+shift+F12'
```
* ### Auto-Clicker Start 
```
./WL_Auto-Clicker.Appimage --start <button> <cps>
```
Starts the clicking process. User can choose between `lmb` and `rmb`, for Left and Right mouse button respectively. The value of cps permitted is between `0.01` and `1,000,000`, however the user receives a confirmation message beyond `2000` cps for reasons described [above](#cps--clicks-per-second).\
Example usage:
```
./WL_Auto-Clicker.Appimage --start lmb 0.02
./WL_Auto-Clicker.Appimage --start rmb 5000
```
## Setup

1. Download the AppImage file from the GitHub Repo (in the Right Panel).
2. Open terminal session in the folder where the AppImage is kept (by default, the Downloads folder).
3. Run `chmod +x WL_Auto-Clicker.AppImage` .Note that just typing `chmod +x WL` and pressing `TAB` should auto-complete the file name. If it doesn't, keep typing out the name until it does.
4. Run `./WL_Auto-Clicker.AppImage`.

The files inside will run, and may ask you to type your password. This is only for some `sudo` commands used to grant permissions.

## Contributions
