/**********************************************
**    This file is part of Lorris
**    http://tasssadar.github.com/Lorris/
**
**    See README and COPYING
***********************************************/

#ifndef NUM_FUNC_H
#define NUM_FUNC_H

#include <QString>
#include <QThread>
#include <QFont>

class QLayout;

#define sizeof_array(a) (sizeof(a)/sizeof(a[0]))
#define delete_vect(vec) \
    for(quint32 i = 0; i < vec.size(); ++i) \
        delete vec[i]; \
    vec.clear();

#ifdef Q_CC_MSVC
    #define PACK_STRUCT(structure) __pragma( pack(push, 1) ) structure __pragma( pack(pop) )
#elif defined(Q_CC_GNU)
    #define PACK_STRUCT(structure) structure __attribute__((packed))
#else
    #error PACK_STRUCT is not defined for this compiler
#endif

class Utils : public QThread
{
    Q_OBJECT
public:
    static QString hexToString(quint8 data, bool withZeroEx = false);
    static QString parseChar(char c);

    static QByteArray convertByteStr(QString str);

    template <typename T> static inline void swapEndian(char *val);
    static void swapEndian(char *val, quint8 size);

    static QString toBase16(quint8 const * first, quint8 const * last);
    static QString toBinary(std::size_t width, int value);

    static void msleep(unsigned long msecs) { QThread::msleep(msecs); }
    static void sleep (unsigned long secs)  { QThread::sleep(secs); }
    static void usleep(unsigned long usecs) { QThread::usleep(usecs); }

    static QFont getMonospaceFont(quint8 size = 9);

    static void showErrorBox(const QString& text, QWidget* parent = 0);

    static void playErrorSound();

    static QString getFontSaveString(const QFont& font);
    static QFont getFontFromString(const QString& str);

    static QString saveWindowParams(QWidget *w);
    static void loadWindowParams(QWidget *w, const QString& param);

    static void deleteLayoutMembers(QLayout *layout);

    static bool isInRect(int px, int py, int rx, int ry, int rw, int rh);
    static bool isInRect(const QPoint& p, int rx, int ry, int rw, int rh);
    static bool isInRect(const QPoint& p, const QPoint& rp, const QPoint& rs);
};

template <typename T>
void Utils::swapEndian(char *val)
{
    for(qint8 i = sizeof(T); i > 0; i -= 2, ++val)
        std::swap(*val, *(val + i - 1));
}

#endif // NUM_FUNC_H
