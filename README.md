# Planned readme

# Manta
An open source game engine with the user in control.

Gives you the power of high level lua, and low level C

manta is written in C and usea the Vulkan api for rendering (openGL planned). 

## How does it work
manta builds and links as a static library. You create a C program and initialize your game and engine in whatever way you like

The advantage with this is this you can configure and make the game engine behave the way you want for your game.

Perhaps you have a game that requires a different kind of collision detection, then you can do that.

You can quickly script modular with the power of lua, but you can also create your custom scripting language just as well with raw C.

Because the engine gives you raw, unaltered, access to C, you can create your own complex structures or algorithms that uses direct file io. This is often not possible in the scripting languages many engines utilize.


