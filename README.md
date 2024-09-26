# while Loop Tutorial

Author: Yoseph Mak

Design: This is a game designed to teach while loops. The game's script itself loops unless you choose not to let it do so.

Text Drawing: Text is not precomputed. Whenever the game is given a paragraph (draw_text_par), it splits the text into lines which are drawn separately with draw_text_line. Each line is at most a certain amount of characters long, but I made sure not to split up a word.

When the game starts, the font library and face are initialized with FreeType, as well as a shaper with HarfBuzz. In draw_text_line, the given line is passed to HarfBuzz to be shaped. Then, I have Character structs which combine this information with FreeType glyph info. Furthermore, at this point I make new textures for the characters in OpenGL. I chose not to reuse textures because the same character could be shaped differently in different text.

Finally, OpenGL draws all the textures using the Character data, and the textures are freed so we don't suffer a memory leak. I did all of this in PlayMode.cpp/hpp, though by my own admission this is not an ideal solution.

There is a slight bug. HarfBuzz doesn't seem to think there is a y offset in any of the text, so characters which should render below like p and y are stuck at the same bottom height. I have no idea how to fix this, unfortunately.

Choices: The script is stored in a text file. If I had more time, I could've turned it into a binary, but this is a case where having the script be visible doesn't change all that much.

The script file alternates lines. The odd lines have three or more numbers. The first is a line index. The second tells you what line(s) you might have next, which could be 1 or more. If that number is greater than 1, then we have a choice. The third and so forth numbers tell us which line(s) to jump to next. This is how choices are implemented; the game will know what lines could have choices and act accordingly. The advantage of this system is that you can jump anywhere, including to a previous line, which is what allows me to loop the script.

Lastly, if there is an extra number at the end, this tells us where to place the text. Another nice script feature allows placing text on the upper or lower level of the screen. The even lines have the actual text content.

Screen Shot:

![Screen Shot](screenshot.png)

How To Play:

Click the mouse button (left click) to advance the text in most cases.

When the text suggests a choice, you should press a number key corresponding to your choice.

The strategy is to play the game until you get the point of the game. Otherwise, you can/should keep replaying.

Sources:
- Open Sans font: https://fonts.google.com/specimen/Open+Sans
- "Rocket" by Kevin MacLeod (incompetech.com / https://incompetech.com/music/royalty-free/music.html)
    Licensed under Creative Commons: By Attribution 4.0 License
    http://creativecommons.org/licenses/by/4.0/
- The background scene is a modified version of "Brunch" by Yixin He. Creative Commons Attribution license.

Other credits:
- https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c and in general the entire HarfBuzz tutorial repository
- https://learnopengl.com/In-Practice/Text-Rendering for advice on getting the FreeType glyphs to work and putting them in OpenGL
- Other comments for code are provided within where necessary.

This game was built with [NEST](NEST.md).

