# Editor Interaction TODO

## Status legend:

- [ ] Not started
- [x] Done

## Todos

### Done

- [x] Make the editor project view more like unity ( do research first)
- [x] Make the heirarchy function more like unity ( do research first )
- [x] Can we integrate Font awesome or some icons pack for all ui icons
- [x] Allow saving the current editor layout so when we open the editor again it looks exactly the same interms of what windows are open and were they are and there size
- [x] I want tuning gameplay to be a separate window in the editor with all gameplay values tunable.
- [x] Lets write a plan for this in the root plans folder: There should only be one spawn component and it shound't have player index it should just randomly spawn players at those points but 2 players shouldnt spawn at the same point also the spawn component should have a setting for team and a check box for if its a faceoff spawn. For faceoff spawns it should do the same as normal spawns but depending on the team for that spawn thats the ones to use depending on whos penality it was to cause the faceoff.
- [x] Lets write a plan for this in the root plans folder: Make gameplay and editor revole around this game play explicitly \_save\docs\GAMEPLAY.md ask questions if u need clarification (anything ur unsure of). There should be no checking. Just stealing and shooting. And then after go thru all docs and modify them to fit the new gameplay.
- [x] Lets write a plan for this in the root plans folder: Add 2d icons for spawns and any other components that wont have meshes just like lights and camera.
- [x] Lets write a plan for this in the root plans folder: Remove all builtin meshes.
- [x] Lets write a plan for this in the root plans folder: Remove all builtin materials and just add materials to data/raw/materials for example materials like glass,metal, etc.
- [x] Lets write a plan for this in the root plans folder: I want to make waypoint makers visible so lets create a prefab for one and allow selecting a waypoint prefab to use in the gameplay settings.

### In Progress

- [ ] I want to add https://github.com/mikke89/RmlUi for in game ui and menus / hud. so like for hud and for home screen, lobby screen and such.

- [ ] Lets write a plan for this in the root plans folder: I want the stick to be a prefab aswell that can be added to a player prefab just like how we add a player prefab to a spawn.

### Not Started

- [ ] Lets write a plan for this in the root plans folder: I want the data to only be for the editor
      and when we build the game client and server the settings should be baked into the applications and
      when u modify the game client settings from within the game like a user would do it saves them to
      a file next to the game client applicaiton. ( but not for editor, editor should always use data).
      So the data should only every be used for the editor. Editor controls the default build settings for client and
      server. And then you should be able to package a release in the editor for the client and also for the server.
      Client and Server should have no external files except for one config file and should be packaged as a single executable.

- [ ] Lets write a plan for this in the root plans folder: Add a setting to the main bar were the
      play button is but on the right side of the screen that lets me change the scale of the editor.

- [ ] Lets write a plan for this in the root plans folder: I want to be able to drag items in the
      project view to move them to different folder. Or maybe a option in the right popup menu
      to select a folder to move it to.

- [ ] Lets write a plan for this in the root plans folder: I added audio files to the raw audio
      folder. Lets add a an audio system and integrate it into the editor and game and also
      add audio as one of the folders for project window.

## Questions

1. How hard would it be to implement something like unity wer i can create and modify scripts
   to define gameplay. This way i can change all functionality from within the editor itself.

## Revisit

- [ ] Lets write a plan for this in the root plans folder: Remove the premade gameobjects for hockey related stuff.s

- [ ] Add hover tooltips for everything in the ui. And make the toolstips for graphics and light settings very details explain what they do and what different values would do.

implement this make sure to use current branch and dont use worktree and only commit ur changes also make sure to update the plan file as we implement it:
finish implementing this make sure to use current branch and dont use worktree and only commit ur changes also make sure to update the plan file as we implement it:
