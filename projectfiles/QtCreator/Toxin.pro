#-------------------------------------------------
#
# Project created by QtCreator 2013-06-25T14:56:46
#
#-------------------------------------------------

defineTest(minQtVersion) {
    maj = $$1
    min = $$2
    patch = $$3
    isEqual(QT_MAJOR_VERSION, $$maj) {
        isEqual(QT_MINOR_VERSION, $$min) {
            isEqual(QT_PATCH_VERSION, $$patch) {
                return(true)
            }
            greaterThan(QT_PATCH_VERSION, $$patch) {
                return(true)
            }
        }
        greaterThan(QT_MINOR_VERSION, $$min) {
            return(true)
        }
    }
    return(false)
}

!minQtVersion(5, 2, 0) {
    error("Cannot build with Qt version $${QT_VERSION}, this project requires at least Qt 5.2.0")
}

QT       += qml quick

TARGET = Toxin
TEMPLATE = app

CONFIG += c++11

INCLUDEPATH += ../../submodules/ProjectTox-Core/toxcore/
win32:INCLUDEPATH += ../../libs/sodium/include/
macx:INCLUDEPATH += /usr/local/include

win32 {
    LIBS += -lWS2_32 ../../libs/sodium/lib/libsodium.a
} else {
    macx {
        LIBS += -L/usr/local/lib -lsodium
    } else {
        LIBS += -lsodium
    }
}

win32:DEFINES += WIN32

SOURCES += \
    ../../Toxin/core.cpp \
    ../../Toxin/settings.cpp \
    ../../Toxin/engine.cpp \
    ../../Toxin/status.cpp \
    ../../Toxin/main.cpp

HEADERS  += \
    ../../Toxin/settings.h \
    ../../Toxin/engine.h \
    ../../Toxin/status.h \
    ../../Toxin/core.h


### ToxCore section. Please keep it alphabetical ###

SOURCES += \
    ../../submodules/ProjectTox-Core/toxcore/assoc.c \
    ../../submodules/ProjectTox-Core/toxcore/DHT.c \
    ../../submodules/ProjectTox-Core/toxcore/friend_requests.c \
    ../../submodules/ProjectTox-Core/toxcore/group_chats.c \
    ../../submodules/ProjectTox-Core/toxcore/LAN_discovery.c \
    ../../submodules/ProjectTox-Core/toxcore/Lossless_UDP.c \
    ../../submodules/ProjectTox-Core/toxcore/Messenger.c \
    ../../submodules/ProjectTox-Core/toxcore/net_crypto.c \
    ../../submodules/ProjectTox-Core/toxcore/network.c \
    ../../submodules/ProjectTox-Core/toxcore/onion.c \
    ../../submodules/ProjectTox-Core/toxcore/onion_announce.c \
    ../../submodules/ProjectTox-Core/toxcore/onion_client.c \
    ../../submodules/ProjectTox-Core/toxcore/ping.c \
    ../../submodules/ProjectTox-Core/toxcore/tox.c \
    ../../submodules/ProjectTox-Core/toxcore/util.c


HEADERS  += \
    ../../submodules/ProjectTox-Core/toxcore/assoc.h \
    ../../submodules/ProjectTox-Core/toxcore/DHT.h \
    ../../submodules/ProjectTox-Core/toxcore/friend_requests.h \
    ../../submodules/ProjectTox-Core/toxcore/group_chats.h \
    ../../submodules/ProjectTox-Core/toxcore/LAN_discovery.h \
    ../../submodules/ProjectTox-Core/toxcore/Lossless_UDP.h \
    ../../submodules/ProjectTox-Core/toxcore/Messenger.h \
    ../../submodules/ProjectTox-Core/toxcore/misc_tools.h \
    ../../submodules/ProjectTox-Core/toxcore/net_crypto.h \
    ../../submodules/ProjectTox-Core/toxcore/network.h \
    ../../submodules/ProjectTox-Core/toxcore/onion.h \
    ../../submodules/ProjectTox-Core/toxcore/onion_announce.h \
    ../../submodules/ProjectTox-Core/toxcore/onion_client.h \
    ../../submodules/ProjectTox-Core/toxcore/ping.h \
    ../../submodules/ProjectTox-Core/toxcore/tox.h \
    ../../submodules/ProjectTox-Core/toxcore/util.h


RESOURCES += \
    ../../Toxin/qml.qrc \
    ../../Toxin/images.qrc

win32:RC_FILE = ../../resources/win32_appicon.rc

# Default rules for deployment.
include(../../Toxin/deployment.pri)
