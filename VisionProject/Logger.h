#pragma once

#include <QObject>
#include <QString>
#include <QMutex>
#include <QFile>

//新增：日志模块
//  1) 带时间戳和级别
//  2) 同时写文件（logs/yyyy-MM-dd.log），关程序也留着
//  3) 用信号把日志送回主线程显示，避免子线程直接碰界面
class Logger : public QObject
{
    Q_OBJECT
public:
    enum Level { Info = 0, Warn, Error };

    static Logger& instance();

    //写一条日志：线程安全，可以从任意线程调用
    void log(Level lv, const QString& msg);

signals:
    //主线程接到这个信号后往界面日志框追加一行
    void newLog(const QString& line);

private:
    Logger();
    ~Logger();
    QString filePathForToday() const;

private:
    QMutex m_mtx;
    QFile  m_file;
    QString m_curDate;
};

//用宏包一层，调用处只写 LOG_INFO("xxx")
#define LOG_INFO(msg)  Logger::instance().log(Logger::Info,  QString("%1").arg(msg))
#define LOG_WARN(msg)  Logger::instance().log(Logger::Warn,  QString("%1").arg(msg))
#define LOG_ERROR(msg) Logger::instance().log(Logger::Error, QString("%1").arg(msg))
