#include "Logger.h"
#include <QDateTime>
#include <QDir>
#include <QMutexLocker>

Logger& Logger::instance()
{
    static Logger s_logger;   //C++11 起局部静态变量初始化是线程安全的
    return s_logger;
}

Logger::Logger()
{
    QDir().mkpath("logs");
    m_curDate = QDate::currentDate().toString("yyyy-MM-dd");
    m_file.setFileName(filePathForToday());
    m_file.open(QIODevice::Append | QIODevice::Text);
}

Logger::~Logger()
{
    if (m_file.isOpen()) {
        m_file.close();
    }
}

QString Logger::filePathForToday() const
{
    return QString("logs/%1.log").arg(QDate::currentDate().toString("yyyy-MM-dd"));
}

void Logger::log(Level lv, const QString& msg)
{
    const char* lvText = "INFO ";
    switch (lv) {
    case Warn:  lvText = "WARN "; break;
    case Error: lvText = "ERROR"; break;
    default:    lvText = "INFO "; break;
    }

    const QString ts = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    const QString line = QString("[%1][%2] %3").arg(ts).arg(lvText).arg(msg);

    {
        QMutexLocker locker(&m_mtx);
        //跨天要换文件
        const QString today = QDate::currentDate().toString("yyyy-MM-dd");
        if (today != m_curDate) {
            m_curDate = today;
            if (m_file.isOpen()) {
                m_file.close();
            }
            m_file.setFileName(filePathForToday());
            m_file.open(QIODevice::Append | QIODevice::Text);
        }
        if (m_file.isOpen()) {
            m_file.write(line.toUtf8());
            m_file.write("\n");
            m_file.flush();
        }
    }

    //信号会自动排队到主线程（接收者在主线程）
    emit newLog(line);
}
