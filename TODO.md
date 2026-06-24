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

### Plans

- [ ] I want to add https://github.com/mikke89/RmlUi for in game ui and menus / hud. so like for hud and for home screen, lobby screen and such.

- [ ] Lets write a plan for this in the root plans folder: There should only be one spawn component and it shound't have player index it should just randomly spawn players at those points but 2 players shouldnt spawn at the same point also the spawn component should have a setting for team and a check box for if its a faceoff spawn. For faceoff spawns it should do the same as normal spawns but depending on the team for that spawn thats the ones to use depending on whos penality it was to cause the faceoff.

- [ ] Lets write a plan for this in the root plans folder: Make gameplay and editor revole around this game play explicitly \_save\docs\GAMEPLAY.md ask questions if u need clarification (anything ur unsure of). There should be no checking. Just stealing and shooting. And then after go thru all docs and modify them to fit the new gameplay.

- [ ] Lets write a plan for this in the root plans folder: Add 2d icons for spawns and any other components that wont have meshes just like lights and camera.

### Not Started

- [ ] I want the data to only be for the editor and when we build the game client and server the settings should be baked into the applications and when u modify the game client settings from within the game like a user would do it saves them to a file next to the game client applicaiton. ( but not for editor, editor should always use data)

- [ ] Remove all builtin meshes and just add meshes to data for basic shapes like cylinder, sphere, cube, torus, capsule, and plane.

- [ ] Remove all builtin materials and just add materials to data for example materials like glass,metal, etc.

- [ ] Remove the premade gameobjects for hockey related stuff.

- [ ] Add hover tooltips for everything in the ui. And make the toolstips for graphics and light settings very details explain what they do and what different values would do.

- [ ] Add a setting to project settings that lets me change the scale of the editor.

- [ ] I want to make waypoint makers visible so lets create a prefab for one and allow selecting a waypoint prefab to use in the gameplay settings.

## Questions

1. How hard would it be to implement something like unity wer i can create and modify scripts
   to define gameplay. This way i can change all functionality from within the editor itself.
