Joe Dobrowolski

First, please note that the .jar is named Assignment8.jar but is for this assignment.  I couldn't figure out IntelliJ and ran out of time, but renaming the file corrupted it so I'm leaving it as is, PLEASE USE THIS .JAR FOR GRADING.
I will go over my new view as well as changes to the previous parts.

Playback View:
This is much like the Visual View but now there's buttons.  See the Javadocs for the Controller and AnimationEditor for more detailed explanations.  The rundown is a play/pause button, a loop toggle (that can be triggered by the L key as well), a speed modifier, an editor gui, and a save file.  Any errors from bad input will show up in a JOptionPanel usually displaying the error message and type of error without crashing the animator and allowing the user to continue trying unhindered.
Load file will load any file with a matching name in the same directory.  Save file allows the user to select an output type and save it with a non-blank name.
The biggest addition is Animation Editor Pro.  There's still some quirks, not all TextBoxes clear when enter is hit, but functionally is appears to work after rigorous testing.  There is ability to add/remove commands and add/remove shapes.
The saved svg/txt files will reflect any changes to the model through this editor.

Controller:
Handles actionCommands from the view buttons to handle their behavior.  I go into a lot of detail in the actionPerformed javadoc in the Controller, and similarly in my taskPerformer object in AnimationEditor for details on every action handled.
I put the animationEditor in the Controller class because, while is is visual, it is not a visualization of the animation per se.  In fact, I would say it is simply a pop-out extension of the Controller that allows more modifications to be handled.

CHANGES:
Shapes:
The parameters for the ellipse store the diameter and top left corner now, the compensations weren't working for scale commands.
SVG still wonky, but I gave up on that.  Any move happening simultaneously with a scale motion would need to constantly recalculate the center point, which got especially tricky as the motion commands don't know if there is a scale happening at the same time when initially storing the commands.  I know how I might approach fixing this, but it would require fundamentally changing my model and I simply didn't have time.  Right now, my Swing views are showing how I want them to.

Model:
I finally got around to getting rid of storing the commands as a String and instead using a class that stored the parameters.  More details in the java doc, but it breaks up the parameters into arrays storing start and end parameters.  I also added a MotionType enum that is not only useful for storing these commands, but allows the AnimationEditor to automatically add the new command types to the list of commands, since that populates based on the enum.  This helped make my code sooo much more efficient.
Also added in removeShape and removeCommand methods to help with the animationEditor.  I did get points off for not including this in Assignment 7, though I don't necessarily agree with this because it wasn't not specifically required and I assumed that we would only be adding to the model at that poiont, not editing after.

View:
I removed the compensation for ellipses because I changed the shape behavior.

I'm proud of my result, but had a miserable time doing this assignment mostly due to how SVG handles ellipses and rewrite huge amounts of code after realizing that I made incorrect assumptions about what was going to come in subsequent parts.  I'm glad that I haven't needed to work due to the quarantine because I literally spent nearly every waking hour doing this assignment over the last few weeks.  Glad to be done with it.


----Part 2 README---- (for extra reference)

All three views extend blank interfaces.  The reason is that at the moment there are no public methods being used, so the interface has nothing to do for now. However, they are still there for when I implement the controller as that will need to access public methods in the views and I can use them for that.

Changes to the model:
I ended up rewriting most of my model from scratch, removing things like Point2D and color objects as parameters to work better with the Builders.  I also implemented a function (with several private helpers) to set the shapes to a given frame.  This essentially starts at the first motion and goes through each command in the list until the start time is after the desired frame.  All commands before this are stored in a map with the type of motion as the key.  This safely stores only the most recent of a certain type of command.  If none of a certain common is found, the default values are used, which is saved when first creating the shape.  I also modified the shapes to store whether or not they are visible - if a frame is before the first appearance, the visibility is set to false and no further calculations are made.
I then have it calculate the state at a given frame, using an equation on any commands that are not complete at that frame and setting it to the end of a command if that command had already ended.

Changes to shapes:
The instructions were, frankly, misleading with handling ellipse parameters and I had a huge setback during the svg view because of this.  It seemed safe to assume that the inputs we were given should be the center and radii, however it was actually the top left corner and the full height and width.  It didn't help that this gave the expected animation in Swing, so I didn't catch this until much later.  I did conversions within my Oval concrete class to correct this.  After that, I was able to fix these offsets in my JPanel and all was good.

JFrameView:
The gist of it is, the timer starts at 0 and each tic it calls on the model to set the frame to that number.  After this, the JPanel is called and refreshed with the new parameters.  I have my animation loop, though there is some commented out code to switch to not loop.  I will make this fully toggleable in Part 3.

TextView:
Pretty simple, just prints out the data from each shape together with their respective commands.

SVGView:
Getting this to work was a nightmare.  After finally getting ellipses to almost work, there were still some quirks with shapes not initializing correctly.  I fixed this by setting the model to the frame where each shape first appears before initializing, which fixed the problem.  The only weird bug which should happen with the way command lines work any ways, is if the Swing animation is called and then the SVG is rendered after, the Swing animation will flicker occasionally when the SVG is changing the current frame.

EasyAnimation:
I was dumb and handled the whole input string as one input with spaces in between, when in reality it should be an array with no spaces unless it's in quotes.  I just stuck them back together as a string, but there's still a potential unaccounted for bug if someone types in a valid input with quotes and multiple spaces.  I will try to rewrite this for the final assignment, but for now I had little time and couldn't really go back and redo all my parsing functions.  I think I'll loop through and when it finds one of the "key" commands, it will send the next element in the list to get checked if it's not also a command.

Last note:
I hate the way I stored the commands in ShapeShell.  I want to completely redo that and store values instead, but I don't know if I'll have time to go back and fix it.  It works and is not a priority, but decoding those strings is a huge waste of code and processing that could be simplified so much.  I chose the string route because in part 7, I didn't know exactly how the view was going to work.  I ended up just making an easy way of closely replicating the format in the text example by making the commands an array of strings with those commands.  I know it's not great design and I would do it differently if I had more time.
