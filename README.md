
# Planned readme

# crescent
An open source game engine that gives you the power of both high level lua, and low level C

crescent is written in C and usea the Vulkan api for rendering (openGL planned). 

## How does it work
crescent builds and links as a static library. You create a C program (or C++), you can then initialize the engine whichever way you like or use th

The advantage with this is this you can configure and make the game engine behave the way you want for your game.

Perhaps you have a game that requires a different kind of collision detection, then you can do that.

You can quickly script modular with the power of lua, but you can also create your custom scripting language just as well with raw C.

Because the engine gives you raw, unaltered, access to C, you can create your own complex structures or algorithms that uses direct file io. This is often not possible in the scripting languages many engines utilize.


