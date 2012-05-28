/*
 * Main Page Data for Doxygen
 */

/**
 * @mainpage
 *
 * The Benthos Dive Computer Library is a C++ plugin framework for interfacing
 * with scuba dive computers.  Modeled loosely after libdivecomputer, the
 * Benthos library implements a true plugin architecture with support for
 * shared library loading, configurable driver arguments and support for reading
 * and writing dive computer settings.  The benthos-dc library is organized into
 * three parts.  The common package contains code for parsing driver arguments,
 * accessing data buffers in both little-endian and big-endian format, and
 * libraries for IrDA and serial port access.  The core package contains the C++
 * libraries which manage dive computer plugins.  The plugins package contains
 * pre-built plugins.
 *
 * Currently benthos-dc includes support for Uwatec Smart dive computers which
 * connect over the IrDA port.  It also includes a libdivecomputer wrapper
 * plugin which allows Benthos to interface with any computer supported by
 * libdivecomputer, albeit with reduced functionality.  See documentation for
 * the libdivecomputer plugin for more details.
 */
