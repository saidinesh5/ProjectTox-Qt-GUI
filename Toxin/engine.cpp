#include "engine.h"

Engine::Engine(QObject *parent) :
    QObject(parent),
    m_core(new Core(this))
{

}
