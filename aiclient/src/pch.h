
#include <QtCore>
#include <QtNetwork>

#ifdef QSGSAICLIENTEXE_BUILDING_QSGSAICLIENTEXE
#define QSGSAICLIENTEXE_EXPORT Q_DECL_EXPORT
#else
#define QSGSAICLIENTEXE_EXPORT Q_DECL_IMPORT
#endif

