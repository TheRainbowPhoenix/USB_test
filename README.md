# JustUI Template Add-in

This folder acts as a base template to jumpstart your JustUI-based projects on the fx-CG50 or ClassPad II. It serves as both a demo of its major features and a foundation text for your custom add-ins.

## Features Included
- **Fullscreen Layout**: Pre-configured `jscene_create_fullscreen` layout with touch compatibility padding.
- **Tab Component**: A three-tab setup built with `jlayout_set_stack`.
  - Tab 1: Simple UI text (`jlabel_create`).
  - Tab 2: Interactive elements (`jbutton_create` that increments a global counter).
  - Tab 3: About text (`jlabel_create`).
- **Navigation Bar**: Tab-switching interface pinned at the bottom with horizontal layout (`jlayout_set_hbox`). Includes an integrated Exit button!

## Starting Development
Use this base template as a boilerplate! Simply edit the code inside `src/main.c` where everything is set up.

Build it with :
```
fxsdk build-cp
```

### CMake Notice
This template's `CMakeLists.txt` is already linked optimally with JustUI.
For reference, it uses:
```cmake
find_package(JustUI 1.0 REQUIRED)
target_link_libraries(myaddin Gint::Gint JustUI::JustUI)
```

Have fun coding your first JustUI project!


