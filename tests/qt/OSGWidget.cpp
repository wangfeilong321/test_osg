#include "OSGWidget.h"
#include "PickHandler.h"

#include <cassert>

#include <stdexcept>
#include <vector>

#include <QDebug>
#include <QKeyEvent>
#include <QPainter>
#include <QWheelEvent>

#include <osgViewer/CompositeViewer>

struct OSGWidget::osg_private
{
	osg_private(osgViewer::GraphicsWindowEmbedded* gwe)
		: graphicsWindow_(gwe)
	{

	}
	
	osgGA::EventQueue* getEventQueue() const;

	osg::ref_ptr<osgViewer::GraphicsWindowEmbedded>			    graphicsWindow_;
	osg::ref_ptr<osgViewer::ViewerBase/*CompositeViewer*/>		viewer_;
};

osgGA::EventQueue* OSGWidget::osg_private::getEventQueue() const
{
	osgGA::EventQueue* eventQueue = graphicsWindow_->getEventQueue();

	if( eventQueue )
		return eventQueue;
	else
		throw std::runtime_error( "Unable to obtain valid event queue");
}

namespace
{

#ifdef WITH_SELECTION_PROCESSING
QRect makeRectangle( const QPoint& first, const QPoint& second )
{
  // Relative to the first point, the second point may be in either one of the
  // four quadrants of an Euclidean coordinate system.
  //
  // We enumerate them in counter-clockwise order, starting from the lower-right
  // quadrant that corresponds to the default case:
  //
  //            |
  //       (3)  |  (4)
  //            |
  //     -------|-------
  //            |
  //       (2)  |  (1)
  //            |

  if( second.x() >= first.x() && second.y() >= first.y() )
    return QRect( first, second );
  else if( second.x() < first.x() && second.y() >= first.y() )
    return QRect( QPoint( second.x(), first.y() ), QPoint( first.x(), second.y() ) );
  else if( second.x() < first.x() && second.y() < first.y() )
    return QRect( second, first );
  else if( second.x() >= first.x() && second.y() < first.y() )
    return QRect( QPoint( first.x(), second.y() ), QPoint( second.x(), first.y() ) );

  // Should never reach that point...
  return QRect();
}
#endif



osg::Node * createScene()
{
	osg::Sphere* sphere    = new osg::Sphere( osg::Vec3( 0.f, 0.f, 0.f ), 0.25f );
	osg::ShapeDrawable* sd = new osg::ShapeDrawable( sphere );
	sd->setColor( osg::Vec4( 1.f, 0.f, 0.f, 1.f ) );
	sd->setName( "A nice sphere" );

	osg::Geode* geode = new osg::Geode;
	geode->addDrawable( sd );

	// Set material for basic lighting and enable depth tests. Else, the sphere
	// will suffer from rendering errors.
	{
		osg::StateSet* stateSet = geode->getOrCreateStateSet();
		osg::Material* material = new osg::Material;

		material->setColorMode( osg::Material::AMBIENT_AND_DIFFUSE );	

		stateSet->setAttributeAndModes( material, osg::StateAttribute::ON );
		stateSet->setMode( GL_DEPTH_TEST, osg::StateAttribute::ON );
	}

	return geode;
}

osgViewer::ViewerBase* createView(int width,int height, osg::Node* scene, osgViewer::GraphicsWindowEmbedded * gwe)
{
	auto viewer_ = new osgViewer::CompositeViewer();

	float aspectRatio = static_cast<float>( width/ 2 ) / static_cast<float>( height );

	osg::Camera* camera = new osg::Camera;
	camera->setViewport( 0, 0, width / 2, height );
	camera->setClearColor( osg::Vec4( 0.f, 0.f, 1.f, 1.f ) );
	camera->setProjectionMatrixAsPerspective( 30.f, aspectRatio, 1.f, 1000.f );
	camera->setGraphicsContext( gwe );

	osgViewer::View* view = new osgViewer::View;
	view->setCamera( camera );
	view->setSceneData( scene );
	view->addEventHandler( new osgViewer::StatsHandler );
#ifdef WITH_PICK_HANDLER
	view->addEventHandler( new PickHandler );
#endif

	osgGA::TrackballManipulator* manipulator = new osgGA::TrackballManipulator;
	manipulator->setAllowThrow( false );

	view->setCameraManipulator( manipulator );

	osg::Camera* sideCamera = new osg::Camera;
	sideCamera->setViewport( width /2, 0,
		width /2, height );

	sideCamera->setClearColor( osg::Vec4( 1.f, 0.f, 1.f, 1.f ) );
	sideCamera->setProjectionMatrixAsPerspective( 30.f, aspectRatio, 1.f, 1000.f );
	sideCamera->setGraphicsContext( gwe );

	osgViewer::View* sideView = new osgViewer::View;
	sideView->setCamera( sideCamera );
	sideView->setSceneData( scene );
	sideView->addEventHandler( new osgViewer::StatsHandler );
	sideView->setCameraManipulator( new osgGA::TrackballManipulator );

	osgViewer::CompositeViewer* cv = dynamic_cast<osgViewer::CompositeViewer*>(/*d_->*/viewer_/*.get()*/);
	cv->addView( view );
	cv->addView( sideView );
	cv->setThreadingModel( osgViewer::CompositeViewer::SingleThreaded );
	cv->realize();

	return viewer_;
}

}

OSGWidget::OSGWidget( QWidget* parent,
                      Qt::WindowFlags f )
  : QOpenGLWidget( parent, f )
  , selectionActive_( false )
  , selectionFinished_( true )
  , d_ (new osg_private( new osgViewer::GraphicsWindowEmbedded( this->x(),
											   this->y(),
											   this->width(),
											   this->height() )))
{


  d_->viewer_  = createView(this->width(),this->height(),createScene(),d_->graphicsWindow_);

  // This ensures that the widget will receive keyboard events. This focus
  // policy is not set by default. The default, Qt::NoFocus, will result in
  // keyboard events that are ignored.
  this->setFocusPolicy( Qt::StrongFocus );
  this->setMinimumSize( 100, 100 );

  // Ensures that the widget receives mouse move events even though no
  // mouse button has been pressed. We require this in order to let the
  // graphics window switch viewports properly.
  this->setMouseTracking( true );
}

OSGWidget::~OSGWidget()
{
	delete d_;
}

void OSGWidget::paintEvent( QPaintEvent* /* paintEvent */ )
{
  this->makeCurrent();

  QPainter painter( this );
  painter.setRenderHint( QPainter::Antialiasing );

  this->paintGL();

#ifdef WITH_SELECTION_PROCESSING
  if( selectionActive_ && !selectionFinished_ )
  {
    painter.setPen( Qt::black );
    painter.setBrush( Qt::transparent );
    painter.drawRect( makeRectangle( selectionStart_, selectionEnd_ ) );
  }
#endif

  painter.end();

  this->doneCurrent();
}

void OSGWidget::paintGL()
{
  d_->viewer_->frame();
}

void OSGWidget::resizeGL( int width, int height )
{
  d_->getEventQueue()->windowResize( this->x(), this->y(), width, height );
  d_->graphicsWindow_->resized( this->x(), this->y(), width, height );

  onResize( width, height );
}

void OSGWidget::keyPressEvent( QKeyEvent* event )
{
  QString keyString   = event->text();
  const char* keyData = keyString.toLocal8Bit().data();

  if( event->key() == Qt::Key_S )
  {
#ifdef WITH_SELECTION_PROCESSING
    selectionActive_ = !selectionActive_;
#endif

    // Further processing is required for the statistics handler here, so we do
    // not return right away.
  }
  else if( event->key() == Qt::Key_D )
  {
    //osgDB::writeNodeFile( *(d_->viewer_)->getView(0)->getSceneData(),
    //                      "/tmp/sceneGraph.osg" );

    return;
  }
  else if( event->key() == Qt::Key_H )
  {
    this->onHome();
    return;
  }

  d_->getEventQueue()->keyPress( osgGA::GUIEventAdapter::KeySymbol( *keyData ) );
}

void OSGWidget::keyReleaseEvent( QKeyEvent* event )
{
  QString keyString   = event->text();
  const char* keyData = keyString.toLocal8Bit().data();

  d_->getEventQueue()->keyRelease( osgGA::GUIEventAdapter::KeySymbol( *keyData ) );
}

void OSGWidget::mouseMoveEvent( QMouseEvent* event )
{
  // Note that we have to check the buttons mask in order to see whether the
  // left button has been pressed. A call to `button()` will only result in
  // `Qt::NoButton` for mouse move events.
  if( selectionActive_ && event->buttons() & Qt::LeftButton )
  {
    selectionEnd_ = event->pos();

    // Ensures that new paint events are created while the user moves the
    // mouse.
    this->update();
  }
  else
  {
    d_->getEventQueue()->mouseMotion( static_cast<float>( event->x() ),
                                        static_cast<float>( event->y() ) );
  }
}

void OSGWidget::mousePressEvent( QMouseEvent* event )
{
  // Selection processing
  if( selectionActive_ && event->button() == Qt::LeftButton )
  {
    selectionStart_    = event->pos();
    selectionEnd_      = selectionStart_; // Deletes the old selection
    selectionFinished_ = false;           // As long as this is set, the rectangle will be drawn
  }

  // Normal processing
  else
  {
    // 1 = left mouse button
    // 2 = middle mouse button
    // 3 = right mouse button

    unsigned int button = 0;

    switch( event->button() )
    {
    case Qt::LeftButton:
      button = 1;
      break;

    case Qt::MiddleButton:
      button = 2;
      break;

    case Qt::RightButton:
      button = 3;
      break;

    default:
      break;
    }

    d_->getEventQueue()->mouseButtonPress( static_cast<float>( event->x() ),
                                             static_cast<float>( event->y() ),
                                             button );
    }
}

void OSGWidget::mouseReleaseEvent(QMouseEvent* event)
{
  // Selection processing: Store end position and obtain selected objects
  // through polytope intersection.
  if( selectionActive_ && event->button() == Qt::LeftButton )
  {
    selectionEnd_      = event->pos();
    selectionFinished_ = true; // Will force the painter to stop drawing the
                               // selection rectangle

    this->processSelection();
  }

  // Normal processing
  else
  {
    // 1 = left mouse button
    // 2 = middle mouse button
    // 3 = right mouse button

    unsigned int button = 0;

    switch( event->button() )
    {
    case Qt::LeftButton:
      button = 1;
      break;

    case Qt::MiddleButton:
      button = 2;
      break;

    case Qt::RightButton:
      button = 3;
      break;

    default:
      break;
    }

    d_->getEventQueue()->mouseButtonRelease( static_cast<float>( event->x() ),
                                               static_cast<float>( event->y() ),
                                               button );
  }
}

void OSGWidget::wheelEvent( QWheelEvent* event )
{
  // Ignore wheel events as long as the selection is active.
  if( selectionActive_ )
    return;

  event->accept();
  int delta = event->delta();

  osgGA::GUIEventAdapter::ScrollingMotion motion = delta > 0 ?   osgGA::GUIEventAdapter::SCROLL_UP
                                                               : osgGA::GUIEventAdapter::SCROLL_DOWN;

  d_->getEventQueue()->mouseScroll( motion );
}

bool OSGWidget::event( QEvent* event )
{
  bool handled = QOpenGLWidget::event( event );

  // This ensures that the OSG widget is always going to be repainted after the
  // user performed some interaction. Doing this in the event handler ensures
  // that we don't forget about some event and prevents duplicate code.
  switch( event->type() )
  {
  case QEvent::KeyPress:
  case QEvent::KeyRelease:
  case QEvent::MouseButtonDblClick:
  case QEvent::MouseButtonPress:
  case QEvent::MouseButtonRelease:
  case QEvent::MouseMove:
  case QEvent::Wheel:
    this->update();
    break;

  default:
    break;
  }

  return handled;
}

void OSGWidget::onHome()
{
  osgViewer::ViewerBase::Views views;
  d_->viewer_->getViews( views );

  for( std::size_t i = 0; i < views.size(); i++ )
  {
    osgViewer::View* view = views.at(i);
    view->home();
  }
}

void OSGWidget::onResize( int width, int height )
{
  std::vector<osg::Camera*> cameras;
  d_->viewer_->getCameras( cameras );

  assert( cameras.size() == 2 );

  cameras[0]->setViewport( 0, 0, this->width() / 2, this->height() );
  cameras[1]->setViewport( this->width() / 2, 0, this->width() / 2, this->height() );
}


void OSGWidget::processSelection()
{
#ifdef WITH_SELECTION_PROCESSING
  QRect selectionRectangle = makeRectangle( selectionStart_, selectionEnd_ );
  int widgetHeight         = this->height();

  double xMin = selectionRectangle.left();
  double xMax = selectionRectangle.right();
  double yMin = widgetHeight - selectionRectangle.bottom();
  double yMax = widgetHeight - selectionRectangle.top();

  osgUtil::PolytopeIntersector* polytopeIntersector
      = new osgUtil::PolytopeIntersector( osgUtil::PolytopeIntersector::WINDOW,
                                          xMin, yMin,
                                          xMax, yMax );

  // This limits the amount of intersections that are reported by the
  // polytope intersector. Using this setting, a single drawable will
  // appear at most once while calculating intersections. This is the
  // preferred and expected behaviour.
  polytopeIntersector->setIntersectionLimit( osgUtil::Intersector::LIMIT_ONE_PER_DRAWABLE );

  osgUtil::IntersectionVisitor iv( polytopeIntersector );

  for( unsigned int viewIndex = 0; viewIndex < viewer_->getNumViews(); viewIndex++ )
  {
    osgViewer::View* view = viewer_->getView( viewIndex );

    if( !view )
      throw std::runtime_error( "Unable to obtain valid view for selection processing" );

    osg::Camera* camera = view->getCamera();

    if( !camera )
      throw std::runtime_error( "Unable to obtain valid camera for selection processing" );

    camera->accept( iv );

    if( !polytopeIntersector->containsIntersections() )
      continue;

    auto intersections = polytopeIntersector->getIntersections();

    for( auto&& intersection : intersections )
      qDebug() << "Selected a drawable:" << QString::fromStdString( intersection.drawable->getName() );
  }
#endif
}
