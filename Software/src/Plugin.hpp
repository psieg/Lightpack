#pragma once

#include <QObject>
#include <QProcess>

#include "common/DebugOut.hpp"

class Plugin : public QObject
{
    Q_OBJECT
public:
    Plugin(QString name, QString path,QObject *parent = 0);
    ~Plugin();

    void Start();
    void Stop();

    QString Name() const;
    QString Guid() const;
    QString Description() const;
    QString Author() const;
    QString Version() const;
    QIcon Icon() const;


    QProcess::ProcessState state() const;
    bool isEnabled() const;
    void setEnabled(bool enable);
    void setPriority(int priority);
    int getPriority() const;


signals:

    void stateChanged(QProcess::ProcessState);
    
public slots:
    
private:

    QString _guid;
    QString _name;
    QString _description;
    QString _author;
    QString _version;
    QString _icon;
    QString _exec;
    QString _arguments;
    QString _pathPlugin;
    QProcess *process;

};


