
QtQuick 2.0 render thread animators -plugin
-------------------------------------------

This plugin is a prototype implementation which provides render thread animations. Render thread
animations are able to run even when main (GUI) thread is blocked. The benefit of this is that
animations appear smooth in (almost) all situations.

To compile plugin, some private Qt 5.0 headers are required. To get the plugin work correctly,
scenegraph adaptation must provide a non-blocking render loop. Default scenegraph implementation
does not currently have this feature enabled. If non-blocking render loop is not available, render
thread animations still work, but the animations are not smooth (just like normal QML animations)
when the main thread gets blocked.

Usage QML examples can be found from the "examples" folder. Each example contains both, normal
animation and render-thread animation and some dummy code which tries to create high CPU-load
so that normal animations are stuttering while render-thread animations should keep running smoothly.

Plugin exports 2 items and 7 animations which are named in the same way as the corresponding QtQuick
elements, so that taking the plugin into use is rather easy. At best cases, only minor modifications
to existing QML code is required.

  "Render-thread animation enabled" -versions of QML types currently are:
  - Item
  - ShaderEffect

  Render thread animation QML types:
  - ColorAnimation
  - NumberAnimation
  - PauseAnimation
  - ParallelAnimation
  - PropertyAnimation
  - SequentialAnimation
  - Vector3DAnimation

Limitations & differences to default QML animation system:

  - Only some of the QML Item properties to be animated inside render thread:

    Item: x, y, opacity, scale, rotation and arbitary transformations (Translate, Scale, Rotation).

    ShaderEffect: In addition to inherited Item properties, custom properties that have GLSL uniform
    bindings can be animated in render thread.

  - Animations are executed asyncronously in the render thread, and the target property value is
    written to the QML property only when animation completes. Only then the change becomes visible
    to the Javascript bindings.

  - In the plugin functionality there are various issues that are not done in the most optimal way
    for various reasons...hopefully the functionality will be included inside QtQuick itself in the
    future Qt releases where it can be done in a more "correct" way.






