#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml>

#include "engine.h"


int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<Engine> ("org.garageresearch.toxin",  0,1, "Engine");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:///qml/main.qml")));

    return app.exec();
}
