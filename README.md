# 2-D-SDL

Project Description
For this game, you will pick an older game (think from the 8 and 16-bit eras).  Something simple like Breakout (https://en.wikipedia.org/wiki/Breakout_(video_game)), Galaga (https://en.wikipedia.org/wiki/Galaga) is my suggestion, but I am open to whatever games you wish to make.  Criteria is



- Must use object-oriented C++.  Separate your classes into separate files.
- Use SDL_image for sprites and other image needs.
- You perform all "physics" calculations yourself without a physics engine.
- Use miniaudio for sounds and/or music.
- You must provide at least one complete level,
- Single player is fine.
- All assets most be licensed appropriately for use, or you can prove you own them.  Include a list of assets and licenses in your submission.
- Have a title screen and a credits screen.


This will not be easy.  Most of you haven't used C++ before.  Until you get comfortable with it, even little things can be hard.  Start early and be consistent in working on it.  You may work in groups of up to 3.  You will need to register your groups on Blackboard.  I have it set so that you should be able to self-enroll.



**Design Considerations**
The wrong design can doom a project.  Make sure you are sticking to design patterns from the book linked on our resources page.  Groups should decide *before anyone starts conding* what the coding standards are.  You should also be sure you have common classes abstracted out.  Don't have one group member create a Koopa and a different one a Goomba without ensuring that both will work exactly the same way.  A better idea would be to create an "Enemy" class with all the common code.



I am not going to force a particular design upon you. However, keep in mind that the design you choose is the one you are stuck coding with.  Spend extra time designing (particularly visually and with UML) and your life will be easier later.  In order to prompt you to focus on your design, I am requiring that I approve your UML design before you start coding.  You can use Lucidchart to create your UML diagrams.  Send them to me with a description (no more than a page) of what each class is used for and how they interact.  If you don't get your design approved, you shouldn't start coding.  Wait until I sign off on it.



Games are meant to be played
We will play each other's games in class.  Be sure your game is playable and all game-ending bugs are fixed.  We will also be evaluating each other's designs.



Please notice that I'm particularly focused on the design you choose for your game.  Many of these "simple" games could be made all in one file, with no objects.  The goal of this assignment is to think about design choices and to learn to evaluate what the pros and cons of choices might be.  I am less interested in you having a fun-to-play game and more interested in you being able to defend your design choices.
