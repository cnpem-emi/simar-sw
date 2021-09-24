#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <Python.h>
#include <stdint.h>

#define DEVICE_NAME0 "/dev/rpmsg_pru30"
#define DEVICE_NAME1 "/dev/rpmsg_pru31"
#define MAX_BUFFER_SIZE 512

char readBuf[MAX_BUFFER_SIZE];

static PyObject *count_pru(PyObject *self, PyObject *args) {
    struct pollfd pfd;
    uint8_t pru, wr_stat;
    uint32_t time_base;
    PyObject* py_counts = PyList_New(4);

    if(!PyArg_ParseTuple(args, "Ih", &time_base, &pru)) return NULL;

    if (pru > 1) {
        PyErr_SetString(PyExc_ValueError, "Invalid PRU selected (only PRUs 0 and 1 are available)");
        return NULL;
    }
            
    switch (pru) {
        case 0:
            pfd.fd = open(DEVICE_NAME0, O_RDWR);
            break;
        case 1:
            pfd.fd = open(DEVICE_NAME1, O_RDWR);
            break;
    }

    if (pfd.fd < 0) {
        PyErr_SetString(PyExc_IOError, "Cannot communicate with selected PRU");
        return NULL;
    }

    wr_stat = write(pfd.fd, "-", 2) == -1;
    usleep(time_base-400); // There is a 400 us offset
    if (wr_stat || write(pfd.fd, "-", 2) == -1) {
        PyErr_SetString(PyExc_IOError, "Cannot communicate with selected PRU");
        return NULL;
    }

    if (read(pfd.fd, readBuf, MAX_BUFFER_SIZE))
    {
        uint8_t offset = 0;
        uint32_t count = 0;
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

