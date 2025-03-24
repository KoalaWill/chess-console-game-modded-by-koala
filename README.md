# chess-console-game-modded-by-koala
 This is a chess console game forked from https://github.com/jeromevonk/chess_console and is edited, added AI (using stockfish.exe), and added a arduino mode by me

<br />

## How to use it....
 Just run the exe file in the b ``` "source\x64\Debug\Chess_console.exe" ```(build with Visual Studio) folder.
 Alternatively, you can also execute the exe file in the build folder(build by Cmake).
 
<br />

## How to modify it with Cmake
 1. Fork it, or download it.
 2. Run ```cmake CmakeLists.txt```.
 3. If errors about the file directory show up, delete the catche file and run the command above again.
 4. Build it via command or press "F7".
 5. Navigate to the build folder
 6. Run the ```.exe``` file.

<br />

## How to change it via Visual Studio
 1. Fork it, or download it.
 2. Open the Visual Studio Solution in ``` "\source\Chess_console.sln" ```.
 3. Edit or add files as you want.
 4. press F5 to build.
 6. the file will automatically be generated at ``` "source\x64\Debug\Chess_console.exe" ```.

<br />

# CAUTION!!!!!!!
### The AI mode can only run when the following files exist in the same folder with the exe.
#### 1. ``` stockfish.exe ```
#### 2. ``` temp_in.txt ```
#### 3. ``` temp_out.txt ```
#### 4. ``` test.bat ```


