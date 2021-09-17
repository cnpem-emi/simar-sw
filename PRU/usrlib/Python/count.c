#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/poll.h>
#include <Python.h>
#include <stdint.h>

#define DEVICE_NAME "/dev/rpmsg_pru30"
#define DEVICE_LOCATION "/sys/class/remoteproc/remoteproc1/state"
#define MAX_BUFFER_SIZE 512

char readBuf[MAX_BUFFER_SIZE];

static PyObject *count_pru(PyObject *self, PyObject *args) {
    struct pollfd pfd;
    uint8_t offset = 0;
    uint32_t time_base, count;
    PyObject* py_counts = PyList_New(4);

    if(!PyArg_ParseTuple(args, "I", &time_base)) return NULL;

    pfd.fd = open(DEVICE_NAME, O_RDWR);

    if (pfd.fd < 0)
    {
        printf("Failed to open %s: %d \n", DEVICE_NAME, pfd.fd);
        return NULL;
    }

    write(pfd.fd, "-", 2);
    usleep(time_base-400); // There is a 400 us offset
    write(pfd.fd, "-", 2);

    if (read(pfd.fd, readBuf, MAX_BUFFER_SIZE))
    {
        for (int i = 0; i < 4; i++) {
            count = (readBuf[offset+3] << 24) | (readBuf[offset+2] << 16) | (readBuf[offset+1] << 8) | readBuf[offset];
            PyObject* py_count = Py_BuildValue("I", count);
            PyList_SetItem(py_counts, i, py_count);
            offset+=4;
        }
    }

    read(pfd.fd, readBuf, MAX_BUFFER_SIZE);
    close(pfd.fd);

    return py_counts;
}

static PyMethodDef CountMethod[] = {
    {"count_pru", count_pru, METH_VARARGS, "Python interface for CountingPRU's counting function"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef countingpru_module = {
    PyModuleDef_HEAD_INIT,
    "countingpru",
    "Python interface for CountingPRU's counting function",
    -1,
    CountMethod
};

PyMODINIT_FUNC PyInit_count_pru(void) {
    return PyModule_Create(&countingpru_module);
}

