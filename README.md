# Classic-Pong
Classic Pong game made for UCR Spring 2015 CS120B

##Video Demonstration
[Classic-Pong YouTube](https://www.youtube.com/watch?v=3tiwWvE0BvI)

##Download and Run

Clone this repository by running in a terminal:

    git clone http://www.github.com/markasfour/Classic-Pong.git

Copy the files found in the `src` directory and add them to a project in [Atmel Studio](http://www.atmel.com/tools/atmelstudio.aspx).

Change the includes for `io.c` and `io.h` to match the path to where you saved them.

Compile the code and program your ATmega1284 microcontroller to have Classic-Pong installed and ready to use.

##Hardware

The hardware used to make this game include:

* ATmega1284 microcontroller

* 8x8 LED matrix

* 5 buttons

* 10k potentiometer

* IEEE ISP adapter

* LCD screen

##How to Play

Navigate the menu using the right two buttons and the bottom left button (`D0`, `D1`, `D2`).

The bottom right button is `1` on the menu, the top right button is `2` on the menu, and the bottom left button is `3` on the menu.

Use the button at the center to perform a soft reset.

In game, use the top button to move your paddle up and use the bottom button to move your paddle down.

Bounce the ball towards your opponent.

If the ball passes your paddle, the opponent scores a point.

The game is best of 7, so the first to 4 wins.

You can check the records in the menu to see how many games were played and how many times each player has won.

The records are saved in `EEPROM`, so even after a power-off, the records will be saved.
