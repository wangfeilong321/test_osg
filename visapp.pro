TEMPLATE = app
win32:TEMPLATE = vcapp

SIMEX_DIR_FULL = $$(SIMEX_DIR)_qt5
DEV_ROOT = $$SIMEX_DIR_FULL
SOLUTION_ROOT = .

include($$SOLUTION_ROOT/base.pri)

include(test_osg.pri)

#############
# include paths

INCLUDEPATH += ./
INCLUDEPATH += \
               $$(SIMEX_DIR_FULL)/src/_Include                \
               $$(SIMEX_DIR_FULL)/src/_Include/network        \
               $$(OSG_DIR)/SPARK-1.5.5/include                \
               $$(BULLET_DIR)/src                             \
               $$PWD/common                                   \
               $$PWD/bada
INCLUDEPATH += $$INCLUDE_PATH
INCLUDEPATH += $$EXT_INCLUDE
INCLUDEPATH += $$EXT_INCLUDE/gmp
INCLUDEPATH += $$_PRO_FILE_PWD_

win32:{
DEFINES -= UNICODE
DEFINES += DISABLE_ROBUST_COMPUTATIONS
DEFINES += BOOST_ALL_NO_LIB
DEFINES += CG_PRIMITIVES
DEFINES += QT_NO_EMIT
DEFINES += QT
}


CONFIG += qt
QT += core gui widgets

SOURCES += \
		tests/qt/QtOSG.cpp \
		tests/qt/MainWindow.cpp 	

SOURCES -= $$PWD/test_osg.cpp       
SOURCES += $$PWD/vis_app.cpp
SOURCES += $$PWD/tests/visapp/visapp1.cpp
SOURCES += $$PWD/tests/visapp/visapp2.cpp
SOURCES += $$PWD/tests/visapp/visapp3.cpp

HEADERS += $$SIMEX_DIR_FULL/src/_Include/ui/tray_icon.h
HEADERS += ./tests/qt/MainWindow.h
			
LIBS += -losgqtwidget

RESOURCES += \
    visapp_resources.qrc

OTHER_FILES += \
    resources/projector.png		


