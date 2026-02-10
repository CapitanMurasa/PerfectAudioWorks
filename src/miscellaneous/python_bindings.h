#ifndef PYTHON_BINDINGS_H
#define PYTHON_BINDINGS_H

class Main_PAW_widget;
class PortaudioThread;
class PythonEventThread;

extern Main_PAW_widget* global_paw_widget;
extern PortaudioThread* global_audiothread;
extern PythonEventThread* global_pyevent;

#endif