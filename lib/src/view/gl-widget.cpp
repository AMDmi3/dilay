/* This file is part of Dilay
 * Copyright © 2015,2016 Alexander Bau
 * Use and redistribute under the terms of the GNU General Public License
 */
#include <QMouseEvent>
#include <QPainter>
#include <glm/glm.hpp>
#include "camera.hpp"
#include "opengl.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "state.hpp"
#include "tool.hpp"
#include "tool/move-camera.hpp"
#include "view/axis.hpp"
#include "view/gl-widget.hpp"
#include "view/main-window.hpp"
#include "view/pointing-event.hpp"
#include "view/util.hpp"

struct ViewGlWidget::Impl {
  typedef std::unique_ptr <State>    StatePtr;
  typedef std::unique_ptr <ViewAxis> AxisPtr;

  ViewGlWidget*   self;
  ViewMainWindow& mainWindow;
  Config&         config;
  Cache&          cache;
  ToolMoveCamera  toolMoveCamera;
  AxisPtr         axis;
  StatePtr       _state;
  bool            tabletPressed;

  Impl (ViewGlWidget* s, ViewMainWindow& mW, Config& cfg, Cache& cch) 
    : self           (s)
    , mainWindow     (mW)
    , config         (cfg)
    , cache          (cch)
    , toolMoveCamera (cfg)
    , axis           (nullptr)
    ,_state          (nullptr)
    , tabletPressed  (false)
  {
    this->self->setAutoFillBackground (false);
  }

  ~Impl () {
    this->self->makeCurrent ();

    this-> axis .reset (nullptr);
    this->_state.reset (nullptr);

    this->self->doneCurrent ();
  }

  State& state () {
    assert (this->_state);
    return *this->_state;
  }

  glm::ivec2 cursorPosition () {
    return ViewUtil::toIVec2 (this->self->mapFromGlobal (QCursor::pos ()));
  }

  void fromConfig () {
    assert (this->axis);

    this->state ().fromConfig ();
    this->axis->fromConfig (this->config);
    this->toolMoveCamera.fromConfig (this->config);
  }

  void initializeGL () {
    OpenGL::initializeFunctions ();

    this-> axis .reset (new ViewAxis (this->config));
    this->_state.reset (new State (this->mainWindow, this->config, this->cache));

    this->self->setMouseTracking (true);
  }

  void paintGL () {
    QPainter painter (this->self);
    painter.beginNativePainting ();

    this->state ().camera ().renderer ().setupRendering ();
    this->state ().scene  ().render (this->state ().camera ());

    if (this->state ().hasTool ()) {
      this->state ().tool ().render ();
    }
    this->axis->render (this->state ().camera ());

    OpenGL::glDisable (OpenGL::DepthTest ()); 
    painter.endNativePainting ();

    this->axis->render (this->state ().camera (), painter);
    this->mainWindow.showNumFaces (this->state ().scene ().numFaces ());
  }

  void resizeGL (int w, int h) {
    this->state ().camera ().updateResolution (glm::uvec2 (w,h));
  }

  void pointingEvent (const ViewPointingEvent& e) {
    if (e.valid ()) {
      if (e.secondaryButton () && e.moveEvent ()) {
        this->toolMoveCamera.moveEvent (this->state (), e);
      }
      else if (e.secondaryButton () && e.pressEvent ()) {
        this->toolMoveCamera.pressEvent (this->state (), e);
      }
      else if (this->state ().hasTool ()) {
        this->state ().handleToolResponse (this->state ().tool ().pointingEvent (e));
      }
    }
  }

  void mouseMoveEvent (QMouseEvent* e) {
    if (this->tabletPressed == false) {
      this->pointingEvent (ViewPointingEvent (*e));
    }
  }

  void mousePressEvent (QMouseEvent* e) {
    if (this->tabletPressed == false) {
      this->pointingEvent (ViewPointingEvent (*e));
    }
  }

  void mouseReleaseEvent (QMouseEvent* e) {
    if (this->tabletPressed == false) {
      this->pointingEvent (ViewPointingEvent (*e));
    }
  }

  void wheelEvent (QWheelEvent* e) {
    if (e->modifiers () == Qt::NoModifier) {
      this->toolMoveCamera.wheelEvent (this->state (), *e);
    }
    else if (this->state ().hasTool ()) {
      this->state ().handleToolResponse (this->state ().tool ().wheelEvent (*e));
    }
  }

  void tabletEvent (QTabletEvent* e) {
    const ViewPointingEvent pointingEvent (*e);

    if (pointingEvent.pressEvent ()) {
      this->tabletPressed = true;
    }
    else if (pointingEvent.releaseEvent ()) {
      this->tabletPressed = false;
    }
    this->pointingEvent (pointingEvent);
  }
};

DELEGATE3_BIG2_SELF (ViewGlWidget, ViewMainWindow&, Config&, Cache&)
GETTER    (ToolMoveCamera&, ViewGlWidget, toolMoveCamera)
DELEGATE  (State&         , ViewGlWidget, state)
DELEGATE  (glm::ivec2     , ViewGlWidget, cursorPosition)
DELEGATE  (void           , ViewGlWidget, fromConfig)
DELEGATE  (void           , ViewGlWidget, initializeGL)
DELEGATE2 (void           , ViewGlWidget, resizeGL, int, int)
DELEGATE  (void           , ViewGlWidget, paintGL)
DELEGATE1 (void           , ViewGlWidget, mouseMoveEvent   , QMouseEvent*)
DELEGATE1 (void           , ViewGlWidget, mousePressEvent  , QMouseEvent*)
DELEGATE1 (void           , ViewGlWidget, mouseReleaseEvent, QMouseEvent*)
DELEGATE1 (void           , ViewGlWidget, wheelEvent       , QWheelEvent*)
DELEGATE1 (void           , ViewGlWidget, tabletEvent      , QTabletEvent*)
